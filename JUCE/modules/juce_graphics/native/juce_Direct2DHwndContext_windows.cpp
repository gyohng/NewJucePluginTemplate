/*
  ==============================================================================

   This file is part of the JUCE framework.
   Copyright (c) Raw Material Software Limited

   JUCE is an open source framework subject to commercial or open source
   licensing.

   By downloading, installing, or using the JUCE framework, or combining the
   JUCE framework with any other source code, object code, content or any other
   copyrightable work, you agree to the terms of the JUCE End User Licence
   Agreement, and all incorporated terms including the JUCE Privacy Policy and
   the JUCE Website Terms of Service, as applicable, which will bind you. If you
   do not agree to the terms of these agreements, we will not license the JUCE
   framework to you, and you must discontinue the installation or download
   process and cease use of the JUCE framework.

   JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
   JUCE Privacy Policy: https://juce.com/juce-privacy-policy
   JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/

   Or:

   You may also use this code under the terms of the AGPLv3:
   https://www.gnu.org/licenses/agpl-3.0.en.html

   THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
   WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

class Presentation
{
public:
    auto getPresentationBitmap() const
    {
        jassert (presentationBitmap != nullptr);
        return presentationBitmap;
    }

    auto getPresentationBitmap (const Rectangle<int>& swapSize, ComSmartPtr<ID2D1DeviceContext1> context)
    {
        if (presentationBitmap != nullptr)
        {
            const auto size = presentationBitmap->GetPixelSize();

            if (size.width != (uint32) swapSize.getWidth() || size.height != (uint32) swapSize.getHeight())
                presentationBitmap = nullptr;
        }

        if (presentationBitmap == nullptr)
        {
            presentationBitmap = Direct2DBitmap::createBitmap (context,
                                                               Image::ARGB,
                                                               { (uint32) swapSize.getWidth(), (uint32) swapSize.getHeight() },
                                                               D2D1_BITMAP_OPTIONS_TARGET);
        }

        return presentationBitmap;
    }

    void setPaintAreas (RectangleList<int> areas)
    {
        paintAreas = std::move (areas);
    }

    auto getPaintAreas() const
    {
        return paintAreas;
    }

private:
    ComSmartPtr<ID2D1Bitmap> presentationBitmap;
    RectangleList<int> paintAreas;
};

class PresentationQueue
{
public:
    Presentation* lockFront()
    {
        const std::scoped_lock lock { mutex };
        displaying = std::exchange (readyToDisplay, nullptr);
        return displaying;
    }

    void unlockFront()
    {
        const std::scoped_lock lock { mutex };
        displaying = nullptr;
    }

    Presentation* lockBack()
    {
        const std::scoped_lock lock { mutex };

        preparing = [&]() -> Presentation*
        {
            for (auto& p : presentations)
                if (&p != displaying && &p != readyToDisplay)
                    return &p;

            return nullptr;
        }();

        return preparing;
    }

    void unlockBack()
    {
        {
            const std::scoped_lock lock { mutex };

            if (readyToDisplay != nullptr)
            {
                // Copy the dirty regions from the newest presentation over the top of the 'ready'
                // presentation, then combine dirty regions.
                // We're effectively combining several frames of dirty regions into one, until
                // the screen update catches up.

                for (const auto& area : preparing->getPaintAreas())
                {
                    D2D1_POINT_2U destPoint { (uint32) area.getX(), (uint32) area.getY() };
                    D2D1_RECT_U sourceRect { (uint32) area.getX(),
                                             (uint32) area.getY(),
                                             (uint32) area.getRight(),
                                             (uint32) area.getBottom() };
                    readyToDisplay->getPresentationBitmap()->CopyFromBitmap (&destPoint, preparing->getPresentationBitmap(), &sourceRect);
                }

                auto areas = readyToDisplay->getPaintAreas();
                areas.add (preparing->getPaintAreas());
                readyToDisplay->setPaintAreas (std::move (areas));
            }
            else
            {
                readyToDisplay = std::exchange (preparing, nullptr);
            }
        }

        SetEvent (wakeEvent.getHandle());
    }

    HANDLE getWakeEvent() const
    {
        return wakeEvent.getHandle();
    }

private:
    WindowsScopedEvent wakeEvent;

    std::mutex mutex;
    std::array<Presentation, 2> presentations;
    Presentation* preparing = nullptr;
    Presentation* readyToDisplay = nullptr;
    Presentation* displaying = nullptr;
};

template <auto lock, auto unlock>
class PresentationQueueLock
{
public:
    PresentationQueueLock() = default;

    explicit PresentationQueueLock (PresentationQueue& q)
        : queue (&q),
          presentation (queue != nullptr ? (queue->*lock)() : nullptr)
    {
    }

    ~PresentationQueueLock()
    {
        if (queue != nullptr)
            (queue->*unlock)();
    }

    PresentationQueueLock (PresentationQueueLock&& other) noexcept
        : queue (std::exchange (other.queue, nullptr)),
          presentation (std::exchange (other.presentation, nullptr))
    {
    }

    PresentationQueueLock& operator= (PresentationQueueLock&& other) noexcept
    {
        PresentationQueueLock { std::move (other) }.swap (*this);
        return *this;
    }

    PresentationQueueLock (const PresentationQueueLock&) = delete;
    PresentationQueueLock& operator= (const PresentationQueueLock&) = delete;

    Presentation* getPresentation() const { return presentation; }

private:
    void swap (PresentationQueueLock& other) noexcept
    {
        std::swap (other.queue, queue);
        std::swap (other.presentation, presentation);
    }

    PresentationQueue* queue = nullptr;
    Presentation* presentation = nullptr;
};

using BackBufferLock = PresentationQueueLock<&PresentationQueue::lockBack, &PresentationQueue::unlockBack>;
using FrontBufferLock = PresentationQueueLock<&PresentationQueue::lockFront, &PresentationQueue::unlockFront>;

struct Direct2DHwndContext::HwndPimpl : public Direct2DGraphicsContext::Pimpl
{
private:
    struct SwapChainThread
    {
        SwapChainThread (Direct2DHwndContext::HwndPimpl& ownerIn,
                         ComSmartPtr<ID2D1Multithread> multithreadIn)
            : owner (ownerIn),
              multithread (multithreadIn),
              swapChainEventHandle (ownerIn.swap.swapChainEvent->getHandle())
        {
        }

        ~SwapChainThread()
        {
            SetEvent (quitEvent.getHandle());
            thread.join();
        }

        BackBufferLock getFreshPresentation()
        {
            return BackBufferLock (queue);
        }

        void notify()
        {
            SetEvent (queue.getWakeEvent());
        }

    private:
        Direct2DHwndContext::HwndPimpl& owner;
        PresentationQueue queue;
        ComSmartPtr<ID2D1Multithread> multithread;
        HANDLE swapChainEventHandle = nullptr;

        WindowsScopedEvent quitEvent;
        std::thread thread { [&] { threadLoop(); } };

        void threadLoop()
        {
            Thread::setCurrentThreadName ("JUCE D2D swap chain thread");

            bool swapChainReady = false;

            const auto serviceSwapChain = [&]
            {
                if (! swapChainReady)
                    return;

                FrontBufferLock frontBufferLock { queue };
                auto* frontBuffer = frontBufferLock.getPresentation();

                if (frontBuffer == nullptr)
                    return;

                JUCE_D2DMETRICS_SCOPED_ELAPSED_TIME (owner.owner.metrics, swapChainThreadTime);

                {
                    ScopedMultithread scopedMultithread { multithread };
                    owner.present (frontBuffer, 0);
                }

                swapChainReady = false;
            };

            for (;;)
            {
                const HANDLE handles[] { swapChainEventHandle, quitEvent.getHandle(), queue.getWakeEvent() };

                const auto waitResult = WaitForMultipleObjects ((DWORD) std::size (handles), handles, FALSE, INFINITE);

                switch (waitResult)
                {
                    case WAIT_OBJECT_0:
                    {
                        swapChainReady = true;
                        serviceSwapChain();
                        break;
                    }

                    case WAIT_OBJECT_0 + 1:
                        return;

                    case WAIT_OBJECT_0 + 2:
                    {
                        serviceSwapChain();
                        break;
                    }

                    case WAIT_FAILED:
                    default:
                        jassertfalse;
                        break;
                }
            }
        }
    };

    SwapChain swap;
    std::unique_ptr<SwapChainThread> swapChainThread;
    BackBufferLock presentation;
    CompositionTree compositionTree;
    UpdateRegion updateRegion;
    RectangleList<int> deferredRepaints;
    Rectangle<int> frameSize;
    std::vector<RECT> dirtyRectangles;
    bool resizing = false;
    int64 lastFinishFrameTicks = 0;

    HWND hwnd = nullptr;

    HRESULT prepare() override
    {
        if (! adapter || ! adapter->direct2DDevice)
        {
            adapter = directX->adapters.getAdapterForHwnd (hwnd);

            if (! adapter)
                return E_FAIL;
        }

        if (! deviceResources.canPaint (adapter))
        {
            if (auto hr = deviceResources.create (adapter); FAILED (hr))
                return hr;
        }

        if (! hwnd || frameSize.isEmpty())
            return E_FAIL;

        if (! swap.canPaint())
        {
            if (auto hr = swap.create (hwnd, frameSize, adapter); FAILED (hr))
                return hr;

            if (auto hr = swap.createBuffer (deviceResources.deviceContext.context); FAILED (hr))
                return hr;
        }

        if (! swapChainThread)
        {
            if (swap.swapChainEvent.has_value())
                swapChainThread = std::make_unique<SwapChainThread> (*this, directX->getD2DMultithread());
        }

        if (! compositionTree.canPaint())
        {
            if (auto hr = compositionTree.create (adapter->dxgiDevice, hwnd, swap.chain); FAILED (hr))
                return hr;
        }

        return S_OK;
    }

    void teardown() override
    {
        compositionTree.release();
        swapChainThread = nullptr;
        swap.release();

        Pimpl::teardown();
    }

    void updatePaintAreas() override
    {
        // Does the entire buffer need to be filled?
        if (swap.state == SwapChain::State::bufferAllocated || resizing)
            deferredRepaints = swap.getSize();

        // If the window alpha is less than 1.0, clip to the union of the
        // deferred repaints so the device context Clear() works correctly
        if (targetAlpha < 1.0f || ! opaque)
            paintAreas = deferredRepaints.getBounds();
        else
            paintAreas = deferredRepaints;
    }

    bool checkPaintReady() override
    {
        // Try not to saturate the message thread; this is a little crude. Perhaps some kind of credit system...
        if (auto now = Time::getHighResolutionTicks(); Time::highResolutionTicksToSeconds (now - lastFinishFrameTicks) < 0.001)
            return false;

        if (presentation.getPresentation() == nullptr)
            presentation = swapChainThread->getFreshPresentation();

        // Paint if:
        //      resources are allocated
        //      deferredRepaints has areas to be painted
        //      the swap chain thread is ready
        bool ready = Pimpl::checkPaintReady();
        ready &= swap.canPaint();
        ready &= compositionTree.canPaint();
        ready &= deferredRepaints.getNumRectangles() > 0 || resizing;
        ready &= presentation.getPresentation() != nullptr;
        return ready;
    }

    JUCE_DECLARE_WEAK_REFERENCEABLE (HwndPimpl)

public:
    HwndPimpl (Direct2DHwndContext& ownerIn, HWND hwndIn, bool opaqueIn)
        : Pimpl (ownerIn, opaqueIn),
          hwnd (hwndIn)
    {
        adapter = directX->adapters.getAdapterForHwnd (hwndIn);
    }

    ~HwndPimpl() override = default;

    HWND getHwnd() const { return hwnd; }

    void handleShowWindow()
    {
        // One of the trickier problems was determining when Direct2D & DXGI resources can be safely created;
        // that's not really spelled out in the documentation.
        // This method is called when the component peer receives WM_SHOWWINDOW
        prepare();

        frameSize = getClientRect();
        deferredRepaints = frameSize;
    }

    Rectangle<int> getClientRect() const
    {
        RECT clientRect;
        GetClientRect (hwnd, &clientRect);

        return Rectangle<int>::leftTopRightBottom (clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);
    }

    Rectangle<int> getFrameSize() override
    {
        return getClientRect();
    }

    ComSmartPtr<ID2D1Image> getDeviceContextTarget() const override
    {
        if (auto* p = presentation.getPresentation())
            return p->getPresentationBitmap (swap.getSize(), deviceResources.deviceContext.context);

        return {};
    }

    void setResizing (bool x)
    {
        resizing = x;
    }

    bool getResizing() const
    {
        return resizing;
    }

    void setSize (Rectangle<int> size)
    {
        if (size == frameSize || size.isEmpty())
            return;

        // Require the entire window to be repainted
        frameSize = size;
        deferredRepaints = size;
        InvalidateRect (hwnd, nullptr, TRUE);

        // Resize/scale the swap chain
        prepare();

        if (auto deviceContext = deviceResources.deviceContext.context)
        {
            ScopedMultithread scopedMultithread { directX->getD2DMultithread() };

            auto hr = swap.resize (size, deviceContext);
            jassert (SUCCEEDED (hr));
            if (FAILED (hr))
                teardown();

            if (swapChainThread)
                swapChainThread->notify();
        }

        clearWindowRedirectionBitmap();
    }

    void addDeferredRepaint (Rectangle<int> deferredRepaint)
    {
        deferredRepaints.add (deferredRepaint);

        JUCE_TRACE_EVENT_INT_RECT (etw::repaint, etw::paintKeyword, snappedRectangle);
    }

    void addInvalidWindowRegionToDeferredRepaints()
    {
        updateRegion.findRECTAndValidate (hwnd);

        // Call addDeferredRepaint for each RECT in the update region to make
        // sure they are snapped properly for DPI scaling
        auto rectArray = updateRegion.getRECTArray();

        for (uint32 i = 0; i < updateRegion.getNumRECT(); ++i)
            addDeferredRepaint (D2DUtilities::toRectangle (rectArray[i]));

        updateRegion.clear();
    }

    void clearWindowRedirectionBitmap()
    {
        if (opaque || swap.state != SwapChain::State::bufferAllocated)
            return;

        deviceResources.deviceContext.createHwndRenderTarget (hwnd);

        // Clear the GDI redirection bitmap using a Direct2D 1.0 render target
        const auto& hwndRenderTarget = deviceResources.deviceContext.hwndRenderTarget;

        if (hwndRenderTarget == nullptr)
            return;

        const auto colorF = D2DUtilities::toCOLOR_F (getBackgroundTransparencyKeyColour());

        RECT clientRect;
        GetClientRect (hwnd, &clientRect);

        const D2D1_SIZE_U size { (uint32) (clientRect.right  - clientRect.left),
                                 (uint32) (clientRect.bottom - clientRect.top) };
        hwndRenderTarget->Resize (size);
        hwndRenderTarget->BeginDraw();
        hwndRenderTarget->Clear (colorF);
        hwndRenderTarget->EndDraw();
    }

    SavedState* startFrame (float dpiScale) override
    {
        if (resizing)
        {
            deferredRepaints = frameSize;
            setSize (getClientRect());
        }

        auto* savedState = Pimpl::startFrame (dpiScale);

        if (savedState == nullptr)
            return nullptr;

        // If a new frame is starting, clear deferredAreas in case repaint is called
        // while the frame is being painted to ensure the new areas are painted on the
        // next frame
        JUCE_TRACE_LOG_D2D_PAINT_CALL (etw::direct2dHwndPaintStart, owner.getFrameId());

        presentation.getPresentation()->setPaintAreas (paintAreas);

        deferredRepaints.clear();

        return savedState;
    }

    HRESULT finishFrame() override
    {
        const ScopeGuard scope { [this]
        {
            presentation = {};
            lastFinishFrameTicks = Time::getHighResolutionTicks();
        } };

        return Pimpl::finishFrame();
    }

    void present (Presentation* paintedPresentation, uint32 flags)
    {
        if (paintedPresentation == nullptr)
            return;

        // Fill out the array of dirty rectangles
        // Compare paintAreas to the swap chain buffer area. If the rectangles in paintAreas are contained
        // by the swap chain buffer area, then mark those rectangles as dirty. DXGI will only keep the dirty rectangles from the
        // current buffer and copy the clean area from the previous buffer.
        // The buffer needs to be completely filled before using dirty rectangles. The dirty rectangles need to be contained
        // within the swap chain buffer.
        JUCE_D2DMETRICS_SCOPED_ELAPSED_TIME (owner.metrics, present1Duration);

        // Allocate enough memory for the array of dirty rectangles
        const auto areas = paintedPresentation->getPaintAreas();
        paintedPresentation->setPaintAreas ({});

        dirtyRectangles.resize ((size_t) areas.getNumRectangles());

        // Fill the array of dirty rectangles, intersecting each paint area with the swap chain buffer
        DXGI_PRESENT_PARAMETERS presentParameters{};

        if (swap.state == SwapChain::State::bufferFilled)
        {
            auto* dirtyRectangle = dirtyRectangles.data();
            auto const swapChainSize = swap.getSize();

            for (const auto& area : areas)
            {
                // If this paint area contains the entire swap chain, then
                // no need for dirty rectangles
                if (area.contains (swapChainSize))
                {
                    presentParameters.DirtyRectsCount = 0;
                    break;
                }

                // Intersect this paint area with the swap chain buffer
                auto intersection = area.getIntersection (swapChainSize);

                if (intersection.isEmpty())
                {
                    // Can't clip to an empty rectangle
                    continue;
                }

                D2D1_POINT_2U destPoint { (uint32) intersection.getX(), (uint32) intersection.getY() };
                D2D1_RECT_U sourceRect { (uint32) intersection.getX(),
                                         (uint32) intersection.getY(),
                                         (uint32) intersection.getRight(),
                                         (uint32) intersection.getBottom() };
                swap.buffer->CopyFromBitmap (&destPoint, paintedPresentation->getPresentationBitmap(), &sourceRect);

                // Add this intersected paint area to the dirty rectangle array (scaled for DPI)
                *dirtyRectangle = D2DUtilities::toRECT (intersection);

                dirtyRectangle++;
                presentParameters.DirtyRectsCount++;
            }

            presentParameters.pDirtyRects = dirtyRectangles.data();
        }

        if (presentParameters.DirtyRectsCount == 0)
        {
            D2D1_POINT_2U destPoint { 0, 0 };
            swap.buffer->CopyFromBitmap (&destPoint, paintedPresentation->getPresentationBitmap(), nullptr);
        }

        // Present the freshly painted buffer
        const auto hr = swap.chain->Present1 (swap.presentSyncInterval, swap.presentFlags | flags, &presentParameters);
        jassertquiet (SUCCEEDED (hr));

        // The buffer is now completely filled and ready for dirty rectangles for the next frame
        swap.state = SwapChain::State::bufferFilled;

        JUCE_TRACE_LOG_D2D_PAINT_CALL (etw::direct2dHwndPaintEnd, owner.getFrameId());
    }

    Image createSnapshot() const
    {
        if (frameSize.isEmpty() || deviceResources.deviceContext.context == nullptr || swap.buffer == nullptr)
            return {};

        // Create the bitmap to receive the snapshot
        D2D1_BITMAP_PROPERTIES1 bitmapProperties{};
        bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
        bitmapProperties.dpiX = bitmapProperties.dpiY = USER_DEFAULT_SCREEN_DPI * owner.getPhysicalPixelScaleFactor();
        bitmapProperties.pixelFormat = swap.buffer->GetPixelFormat();

        const D2D_SIZE_U size { (UINT32) frameSize.getWidth(), (UINT32) frameSize.getHeight() };

        ComSmartPtr<ID2D1Bitmap1> snapshot;

        if (const auto hr = deviceResources.deviceContext.context->CreateBitmap (size, nullptr, 0, bitmapProperties, snapshot.resetAndGetPointerAddress()); FAILED (hr))
            return {};

        const ScopedMultithread scope { directX->getD2DMultithread() };

        swap.chain->Present (0, DXGI_PRESENT_DO_NOT_WAIT);

        // Copy the swap chain buffer to the bitmap snapshot
        D2D_POINT_2U p { 0, 0 };
        const auto sourceRect = D2DUtilities::toRECT_U (frameSize);

        if (const auto hr = snapshot->CopyFromBitmap (&p, swap.buffer, &sourceRect); FAILED (hr))
            return {};

        auto pixelData = Direct2DPixelData::fromDirect2DBitmap (snapshot);
        Image result { pixelData };

        swap.chain->Present (0, DXGI_PRESENT_DO_NOT_WAIT);

        return result;
    }
};

//==============================================================================
Direct2DHwndContext::Direct2DHwndContext (void* windowHandle, bool opaque)
{
   #if JUCE_DIRECT2D_METRICS
    metrics = new Direct2DMetrics { Direct2DMetricsHub::getInstance()->lock,
                                    "HWND " + String::toHexString ((pointer_sized_int) windowHandle),
                                    windowHandle };
    Direct2DMetricsHub::getInstance()->add (metrics);
   #endif

    pimpl = std::make_unique<HwndPimpl> (*this, reinterpret_cast<HWND> (windowHandle), opaque);
    updateSize();
}

Direct2DHwndContext::~Direct2DHwndContext()
{
   #if JUCE_DIRECT2D_METRICS
    Direct2DMetricsHub::getInstance()->remove (metrics);
   #endif
}

void* Direct2DHwndContext::getHwnd() const noexcept
{
    return pimpl->getHwnd();
}

Direct2DGraphicsContext::Pimpl* Direct2DHwndContext::getPimpl() const noexcept
{
    return pimpl.get();
}

void Direct2DHwndContext::handleShowWindow()
{
    pimpl->handleShowWindow();
}

void Direct2DHwndContext::setWindowAlpha (float alpha)
{
    pimpl->setTargetAlpha (alpha);
}

void Direct2DHwndContext::setResizing (bool x)
{
    pimpl->setResizing (x);
}

bool Direct2DHwndContext::getResizing() const
{
    return pimpl->getResizing();
}

void Direct2DHwndContext::setSize (int width, int height)
{
    pimpl->setSize ({ width, height });
}

void Direct2DHwndContext::updateSize()
{
    pimpl->setSize (pimpl->getClientRect());
}

void Direct2DHwndContext::addDeferredRepaint (Rectangle<int> deferredRepaint)
{
    pimpl->addDeferredRepaint (deferredRepaint);
}

void Direct2DHwndContext::addInvalidWindowRegionToDeferredRepaints()
{
    pimpl->addInvalidWindowRegionToDeferredRepaints();
}

Image Direct2DHwndContext::createSnapshot() const
{
    return pimpl->createSnapshot();
}

void Direct2DHwndContext::clearTargetBuffer()
{
    // For opaque windows, clear the background to black with the window alpha
    // For non-opaque windows, clear the background to transparent black
    // In either case, add a transparency layer if the window alpha is less than 1.0
    pimpl->getDeviceContext()->Clear (pimpl->backgroundColor);

    if (pimpl->targetAlpha < 1.0f)
        beginTransparencyLayer (pimpl->targetAlpha);
}

} // namespace juce
