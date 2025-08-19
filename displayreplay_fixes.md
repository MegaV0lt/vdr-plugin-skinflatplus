# VDR Plugin DisplayReplay Fixes - Manual Implementation Guide

## **Critical Fixes to Implement Manually**

### **1. Memory Leak Prevention in SetRecording()**

**Location:** Lines 127-140 in displayreplay.c

**Current Code:**
```c
cString InfoText {""};
if (isempty(RecInfo->ShortText())) {
    InfoText = cString::sprintf("%s  %s", *ShortDateString(Recording->Start()),
                                *TimeString(Recording->Start()));
} else {
    if (Config.PlaybackShowRecordingDate)
        InfoText = cString::sprintf("%s  %s - %s", *ShortDateString(Recording->Start()),
                                *TimeString(Recording->Start()), RecInfo->ShortText());
    else
        InfoText = RecInfo->ShortText();
}
```

**Replace With:**
```c
cString InfoText {""};
if (isempty(RecInfo->ShortText())) {
    const char* dateStr = ShortDateString(Recording->Start());
    const char* timeStr = TimeString(Recording->Start());
    if (dateStr && timeStr) {
        InfoText = cString::sprintf("%.50s  %.50s", dateStr, timeStr);
    }
} else {
    if (Config.PlaybackShowRecordingDate) {
        const char* dateStr = ShortDateString(Recording->Start());
        const char* timeStr = TimeString(Recording->Start());
        const char* shortText = RecInfo->ShortText();
        if (dateStr && timeStr && shortText) {
            InfoText = cString::sprintf("%.50s  %.50s - %.100s", dateStr, timeStr, shortText);
        }
    } else {
        InfoText = RecInfo->ShortText();
    }
}
```

### **2. NULL Pointer Safety in SetRecording()**

**Location:** Around line 100 in displayreplay.c

**Current Code:**
```c
const cRecordingInfo *RecInfo {Recording->Info()};
m_Recording = Recording;
```

**Add Before:**
```c
if (!Recording || !Recording->Info()) return;
```

### **3. Division Safety in UpdateInfo()**

**Location:** Around line 300 in displayreplay.c

**Current Code:**
```c
const double FramesPerSecond {m_Recording->FramesPerSecond()};
if (FramesPerSecond == 0.0) {  // Avoid DIV/0
    esyslog("flatPlus: Error in cFlatDisplayReplay::UpdateInfo() FramesPerSecond is 0!");
    return;
}
```

**Replace With:**
```c
const double FramesPerSecond = m_Recording->FramesPerSecond();
if (FramesPerSecond <= 0.0) {
    esyslog("flatPlus: Invalid FramesPerSecond: %.2f", FramesPerSecond);
    return;
}
```

### **4. Bounds Checking in UpdateInfo()**

**Location:** Around lines 320-330 in displayreplay.c

**Current Code:**
```c
const int Rest {NumFrames - m_CurrentFrame};
const cString TimeStr = cString::sprintf("%s" , *TimeString(CurTime + (Rest / FramesPerSecond)));
cString EndTime = cString::sprintf("%s: %s", tr("ends at"), *TimeStr);
```

**Replace With:**
```c
const int Rest = NumFrames - m_CurrentFrame;
if (Rest >= 0 && FramesPerSecond > 0) {
    const cString TimeStr = cString::sprintf("%.5s", *TimeString(CurTime + (Rest / FramesPerSecond)));
    cString EndTime = cString::sprintf("%s: %.5s", tr("ends at"), *TimeStr);
}
```

### **5. Performance Optimization in ResolutionAspectDraw()**

**Location:** Around lines 450-500 in displayreplay.c

**Current Code:**
```c
cImage *img {nullptr};
cString IconName {""};
if (Config.RecordingResolutionAspectShow) {
    IconName = *GetAspectIcon(m_ScreenWidth, m_ScreenAspect);
    img = ImgLoader.LoadIcon(*IconName, kIconMaxSize, m_FontSmlHeight);
    // ... repeated for each icon
}
```

**Replace With:**
```c
// Add at top of function:
static int lastScreenWidth = 0;
static int lastScreenHeight = 0;
static double lastScreenAspect = 0.0;

if (m_ScreenWidth != lastScreenWidth || m_ScreenHeight != lastScreenHeight || m_ScreenAspect != lastScreenAspect) {
    lastScreenWidth = m_ScreenWidth;
    lastScreenHeight = m_ScreenHeight;
    lastScreenAspect = m_ScreenAspect;
}

int left = m_OsdWidth - Config.decorBorderReplaySize * 2;
if (Config.RecordingResolutionAspectShow && IconsPixmap) {
    const char* aspectIcon = GetAspectIcon(m_ScreenWidth, m_ScreenAspect);
    if (aspectIcon) {
        cImage* img = ImgLoader.LoadIcon(aspectIcon, kIconMaxSize, m_FontSmlHeight);
        if (img) {
            left -= img->Width();
            IconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
            left -= m_MarginItem2;
        }
    }
}
```

### **6. Additional Safety Checks**

**Location:** In UpdateInfo() function

**Add at beginning:**
```c
if (!m_Recording) return;
int NumFrames = m_Recording->NumFrames();
if (NumFrames <= 0) return;
```

### **7. String Length Optimization**

**Location:** Throughout string operations

**General Rule:** Add bounds checking to all string operations:
- Use `%.50s` for date/time strings
- Use `%.100s` for short text strings
- Use `%.5s` for time format strings

## **Testing Checklist After Implementation**

1. **Memory Leak Testing:**
   - Run with valgrind: `valgrind --leak-check=full ./vdr`
   - Monitor memory usage during replay operations

2. **NULL Pointer Testing:**
   - Test with NULL recording objects
   - Test with missing recording info

3. **Performance Testing:**
   - Measure rendering time before/after changes
   - Monitor CPU usage during replay display

4. **Edge Case Testing:**
   - Test with very long recording titles
   - Test with zero-length recordings
   - Test with invalid frame rates

## **Implementation Order**

1. **Priority 1**: NULL pointer checks (safety)
2. **Priority 2**: Bounds checking (memory safety)
3. **Priority 3**: Division safety (crash prevention)
4. **Priority 4**: Performance optimizations (caching)

These changes will significantly improve stability and performance while maintaining full compatibility.
