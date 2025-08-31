// ============================================================================
// File: src/gestures/GestureEngine.cpp
// ----------------------------------------------------------------------------
#include "GestureEngine.h"
#include <math.h>

static float fdist(int x1,int y1,int x2,int y2){ float dx=x2-x1, dy=y2-y1; return sqrtf(dx*dx+dy*dy); }
static float fangle(int x1,int y1,int x2,int y2){ return atan2f((float)(y2-y1),(float)(x2-x1)); }

void GestureEngine::reset(){ _lastTapTime=0; _twoActive=false; _twoCommitted=false; }

GestureEvent GestureEngine::process(const TouchPoint pts[MAX_TOUCH_POINTS], uint8_t active){
  GestureEvent g; g.type=GestureType::None; g.timestamp=millis(); g.finger_count=active;
  unsigned long now = g.timestamp;

  // Zähle aktive & finde Indexe der zwei ersten
  int ids[3]; int k=0; for (int i=0;i<MAX_TOUCH_POINTS;i++) if (pts[i].active && k<3) ids[k++]=i;

  if (active==0){ _twoActive=false; _twoCommitted=false; return g; }

  // Long-Press / Tap / Swipe für 1-Finger
  if (active==1){
    auto &tp = pts[ids[0]];
    float dx = (float)tp.x - (float)tp.start_x;
    float dy = (float)tp.y - (float)tp.start_y;
    float adx=fabsf(dx), ady=fabsf(dy);

    // Long-Press (einmalig)
    if (!tp.long_press_fired && (now - tp.touch_start) >= LONG_PRESS_DURATION && (adx<=TAP_MAX_MOVEMENT && ady<=TAP_MAX_MOVEMENT)){
      g.type = GestureType::LongPress; g.x=tp.x; g.y=tp.y; g.value=(float)(now - tp.touch_start);
      // Hinweis: Rücksetzen erfolgt in Touch-Layer
      return g;
    }

    // Swipe – klare Achse
    if (fdist(tp.x,tp.y,tp.start_x,tp.start_y) >= SWIPE_MIN_DISTANCE){
      if (adx > ady*SWIPE_AXIS_RATIO) g.type = (dx>0)?GestureType::SwipeRight:GestureType::SwipeLeft;
      else if (ady > adx*SWIPE_AXIS_RATIO) g.type = (dy>0)?GestureType::SwipeDown:GestureType::SwipeUp;
      if (g.type!=GestureType::None){ g.x=tp.x; g.y=tp.y; g.value=fdist(tp.x,tp.y,tp.start_x,tp.start_y); return g; }
    }

    // Tap (Release-Handler wäre exakter; hier Live‑Decision, ok für Demo)
    if ((now - tp.touch_start) <= TAP_MAX_DURATION && adx<=TAP_MAX_MOVEMENT && ady<=TAP_MAX_MOVEMENT){
      // Double‑Tap Fenster prüfen
      if (_lastTapTime>0 && (now - _lastTapTime) <= DOUBLE_TAP_INTERVAL){
        g.type = GestureType::DoubleTap; g.x=tp.x; g.y=tp.y; g.value=0; _lastTapTime=0; return g;
      } else {
        g.type = GestureType::Tap; g.x=tp.x; g.y=tp.y; g.value=0; _lastTapTime=now; _lastTapX=tp.x; _lastTapY=tp.y; return g;
      }
    }
    return g;
  }

  // Zwei-Finger aktiv: Pinch/Rotate
  if (active==2){
    auto &a = pts[ids[0]]; auto &b = pts[ids[1]];
    float d = fdist(a.x,a.y,b.x,b.y);
    float ang = fangle(a.x,a.y,b.x,b.y);

    if (!_twoActive){ _twoActive=true; _twoCommitted=false; _twoStartDist=d; _twoStartAngle=ang; return g; }

    float dd = d - _twoStartDist;
    float da = ang - _twoStartAngle; while (da>M_PI) da-=2*M_PI; while (da<-M_PI) da+=2*M_PI;

    uint16_t cx = (a.x + b.x)/2, cy = (a.y + b.y)/2;

    if (!_twoCommitted && fabsf(dd) >= PINCH_THRESHOLD){
      g.type = (dd<0)?GestureType::PinchIn:GestureType::PinchOut; g.x=cx; g.y=cy; g.value=fabsf(dd); _twoCommitted=true; return g;
    }
    if (!_twoCommitted && fabsf(da) >= (ROTATE_THRESHOLD * (M_PI/180.f))){
      g.type = (da>0)?GestureType::RotateCCW:GestureType::RotateCW; g.x=cx; g.y=cy; g.value=fabsf(da); _twoCommitted=true; return g;
    }
    return g;
  }

  // Drei Finger → Three-Finger-Tap (sehr kurze Haltezeit / Demo: sofort)
  if (active>=3){ g.type=GestureType::ThreeFingerTap; g.x=0; g.y=0; g.value=0; return g; }
  return g;
}
