#include "GestureEngine.h"
#include <math.h>

static inline float fdist(int x1,int y1,int x2,int y2){
  float dx=x2-x1, dy=y2-y1; return sqrtf(dx*dx+dy*dy);
}
static inline float fangle(int x1,int y1,int x2,int y2){
  return atan2f((float)(y2-y1),(float)(x2-x1));
}

void GestureEngine::reset(){
  one_active = false; one_committed = false;
  two_active = false; two_committed = false;
  lastTapTime = 0;
}

GestureEvent GestureEngine::process(const TouchPoint pts[MAX_TOUCH_POINTS], uint8_t active){
  GestureEvent g; g.type = GestureType::None; g.timestamp = millis(); g.finger_count = active;

  // Finde erste beiden aktiven IDs
  int id[3]; int k=0;
  for (int i=0;i<MAX_TOUCH_POINTS && k<3;i++) if (pts[i].active) id[k++]=i;

  // 0 Finger: Aufräumen + ggf. Tap / Swipe finalisieren
  if (active == 0){
    if (one_active){
      auto dt = g.timestamp - one_t0;
      float dx = (float)one_lx - (float)one_sx;
      float dy = (float)one_ly - (float)one_sy;
      float adx=fabsf(dx), ady=fabsf(dy);

      if (!one_committed){
        // Tap/DoubleTap
        if (dt <= TAP_MAX_DURATION && adx <= TAP_MAX_MOVEMENT && ady <= TAP_MAX_MOVEMENT){
          if (lastTapTime > 0 && (g.timestamp - lastTapTime) <= DOUBLE_TAP_INTERVAL){
            g.type = GestureType::DoubleTap; g.x = one_lx; g.y = one_ly;
            lastTapTime = 0;
          } else {
            g.type = GestureType::Tap; g.x = one_lx; g.y = one_ly;
            lastTapTime = g.timestamp; lastTapX = one_lx; lastTapY = one_ly;
          }
        } else {
          // Swipe final auf Release entscheiden
          if (fdist(one_lx,one_ly,one_sx,one_sy) >= SWIPE_MIN_DISTANCE){
            if (adx > ady * SWIPE_AXIS_RATIO) g.type = (dx>0)?GestureType::SwipeRight:GestureType::SwipeLeft;
            else if (ady > adx * SWIPE_AXIS_RATIO) g.type = (dy>0)?GestureType::SwipeDown:GestureType::SwipeUp;
            if (g.type != GestureType::None){ g.x=one_lx; g.y=one_ly; g.value=fdist(one_lx,one_ly,one_sx,one_sy); }
          }
        }
      }
    }
    // 2-Finger: Two-Finger-Tap, wenn NICHTS anderes committed wurde
    if (two_active && !two_committed){
      if (g.type == GestureType::None){ // nichts anderes ausgelöst
        g.type = GestureType::TwoFingerTap; g.x = two_cx; g.y = two_cy;
      }
    }
    // Reset
    one_active=false; one_committed=false;
    two_active=false; two_committed=false;
    return g;
  }

  // 1 Finger aktiv – Zustand fortschreiben
  if (active == 1){
    auto &p = pts[id[0]];
    if (!one_active){
      one_active = true; one_committed=false;
      one_t0 = g.timestamp; one_sx = one_lx = p.x; one_sy = one_ly = p.y;
    } else {
      one_lx = p.x; one_ly = p.y;
      // Long-Press einmalig
      if (!one_committed){
        auto dt = g.timestamp - one_t0;
        float adx=fabsf((float)one_lx - (float)one_sx);
        float ady=fabsf((float)one_ly - (float)one_sy);
        if (dt >= LONG_PRESS_DURATION && adx <= TAP_MAX_MOVEMENT && ady <= TAP_MAX_MOVEMENT){
          g.type = GestureType::LongPress; g.x = one_lx; g.y = one_ly; g.value = dt;
          one_committed = true; return g;
        }
      }
    }
    return g; // noch kein finales Event (Tap/Swipe erst bei Release)
  }

  // 2 Finger aktiv – Pinch/Rotate "einmalig"
  if (active >= 2){
    auto &a = pts[id[0]], &b = pts[id[1]];
    float d = fdist(a.x,a.y,b.x,b.y);
    float ang = fangle(a.x,a.y,b.x,b.y);
    uint16_t cx = (a.x + b.x)/2, cy = (a.y + b.y)/2;

    if (!two_active){
      two_active = true; two_committed = false;
      two_d0 = d; two_a0 = ang; two_t0 = g.timestamp; two_cx = cx; two_cy = cy;
      // 1-Finger Kandidat pausieren, wird ggf. als Tap/Swipe verworfen
      return g;
    }
    if (!two_committed){
      float dd = d - two_d0;
      float da = ang - two_a0; while (da >  M_PI) da -= 2*M_PI; while (da < -M_PI) da += 2*M_PI;

      if (fabsf(dd) >= PINCH_THRESHOLD){
        g.type = (dd<0)?GestureType::PinchIn:GestureType::PinchOut; g.x=cx; g.y=cy; g.value=fabsf(dd);
        two_committed = true; return g;
      }
      if (fabsf(da) >= (ROTATE_THRESHOLD * (M_PI/180.f))){
        g.type = (da>0)?GestureType::RotateCCW:GestureType::RotateCW; g.x=cx; g.y=cy; g.value=fabsf(da);
        two_committed = true; return g;
      }
    }
    // Während 2-Finger aktiv keine 1-Finger-Entscheidung
    one_active = false; one_committed = false;
    return g;
  }

  return g;
}
