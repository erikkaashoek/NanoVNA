/*
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * The software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

//#ifdef __SI4432__
#include "si4432.h"		// comment out for simulation
//#endif
#include "stdlib.h"

#pragma GCC push_options
#pragma GCC optimize ("Os")


//#define __DEBUG_AGC__         If set the AGC value will be shown in the stored trace and FAST_SWEEP rmmode will be disabled
#ifdef __DEBUG_AGC__
#ifdef __FAST_SWEEP__
#undef __FAST_SWEEP__
#endif
#endif
// uint8_t dirty = true;
int scandirty = true;

setting_t setting;
freq_t frequencies[POINTS_COUNT];

uint16_t actual_rbw_x10 = 0;
uint16_t vbwSteps = 1;
freq_t minFreq = 0;
freq_t maxFreq = 520000000;

static freq_t old_freq[4] = { 0, 0, 0, 0};
static freq_t real_old_freq[4] = { 0, 0, 0, 0};
//int setting.refer = -1;  // Off by default
const int reffer_freq[] = {30000000, 15000000, 10000000, 4000000, 3000000, 2000000, 1000000};

int in_selftest = false;

void update_min_max_freq(void)
{
  switch(setting.mode) {
  case M_LOW:
    minFreq = 0;
    maxFreq = DEFAULT_MAX_FREQ;
    break;
#ifdef __ULTRA__
  case M_ULTRA:
    minFreq = 674000000;
    maxFreq = 4300000000;
    break;
#endif
  case M_GENLOW:
    minFreq = 0;
    maxFreq = DEFAULT_MAX_FREQ;
    break;
  case M_HIGH:
    minFreq = HIGH_MIN_FREQ_MHZ * 1000000;
    maxFreq = HIGH_MAX_FREQ_MHZ * 1000000;
    break;
  case M_GENHIGH:
    minFreq = 240000000;
    maxFreq = 960000000;
    break;
  }
}

void reset_settings(int m)
{
//  strcpy((char *)spi_buffer, dummy);
  setting.mode = m;
  update_min_max_freq();
  sweep_mode |= SWEEP_ENABLE;
  setting.unit_scale_index = 0;
  setting.unit_scale = 1;
  setting.unit = U_DBM;
  set_scale(10);
  set_reflevel(-10);
  setting.attenuate_x2 = 0;
  setting.rbw_x10 = 0;
  setting.average = 0;
  setting.harmonic = 0;
  setting.show_stored = 0;
  setting.auto_attenuation = false;
  setting.subtract_stored = 0;
  setting.normalize_level = 0.0;
  setting.lo_drive=13;
  setting.rx_drive=8;
  setting.atten_step = 0;       // Only used in low output mode
  setting.agc = S_AUTO_ON;
  setting.lna = S_AUTO_OFF;
  setting.tracking = false;
  setting.modulation = MO_NONE;
  setting.modulation_frequency = 1000;
  setting.step_delay = 0;
  setting.offset_delay = 0;
  setting.step_delay_mode = SD_NORMAL;
  setting.vbw_x10 = 0;
  setting.auto_reflevel = true;     // Must be after SetReflevel
  setting.decay=20;
  setting.attack=1;
  setting.noise=5;
  setting.below_IF = S_AUTO_OFF;
  setting.repeat = 1;
  setting.tracking_output = false;
  setting.measurement = M_OFF;
  setting.frequency_IF = DEFAULT_IF;
  setting.auto_IF = true;
  set_offset(0.0);  // This also updates the help text!!!!!
  //setting.offset = 0.0;
  setting.trigger = T_AUTO;
  setting.trigger_direction = T_UP;
  setting.trigger_mode = T_MID;
  setting.fast_speedup = 0;
  setting.level_sweep = 0.0;
  setting.level = -15.0;
  setting.trigger_level = -150.0;
  setting.linearity_step = 0;
  trace[TRACE_STORED].enabled = false;
  trace[TRACE_TEMP].enabled = false;
//  setting.refer = -1;             // do not reset reffer when switching modes
  setting.mute = true;
#ifdef __SPUR__
#ifdef TINYSA4
  setting.spur_removal = S_AUTO_OFF;
#else
  setting.spur_removal = S_OFF;
#endif
  setting.mirror_masking = 0;
  setting.slider_position = 0;
  setting.slider_span = 100000;

#endif
  switch(m) {
  case M_LOW:
    set_sweep_frequency(ST_START, minFreq);
    set_sweep_frequency(ST_STOP, maxFreq);
    setting.attenuate_x2 = 60;
    setting.auto_attenuation = true;
    setting.sweep_time_us = 0;
    setting.correction_frequency = config.low_correction_frequency;
    setting.correction_value = config.low_correction_value;
    break;
#ifdef __ULTRA__
  case M_ULTRA:
    set_sweep_frequency(ST_START, minFreq);
    set_sweep_frequency(ST_STOP, maxFreq);
    setting.attenuate_x2 = 0;
    setting.sweep_time_us = 0;
    break;
#endif
  case M_GENLOW:
    setting.rx_drive=8;
	setting.lo_drive=13;
    set_sweep_frequency(ST_CENTER, 10000000);
    set_sweep_frequency(ST_SPAN, 0);
    setting.sweep_time_us = 10*ONE_SECOND_TIME;
    setting.step_delay_mode = SD_FAST;
    setting.correction_frequency = config.low_correction_frequency;
    setting.correction_value = config.low_correction_value;
    break;
  case M_HIGH:
    set_sweep_frequency(ST_START, minFreq);
    set_sweep_frequency(ST_STOP,  maxFreq);
    setting.sweep_time_us = 0;
    setting.correction_frequency = config.high_correction_frequency;
    setting.correction_value = config.high_correction_value;
    break;
  case M_GENHIGH:
    setting.lo_drive=8;
    set_sweep_frequency(ST_CENTER, 300000000);
    set_sweep_frequency(ST_SPAN, 0);
    setting.sweep_time_us = 10*ONE_SECOND_TIME;
    setting.step_delay_mode = SD_FAST;
    setting.correction_frequency = config.high_correction_frequency;
    setting.correction_value = config.high_correction_value;
    break;
  }
  for (uint8_t i = 0; i< MARKERS_MAX; i++) {
    markers[i].enabled = M_DISABLED;
    markers[i].mtype = M_NORMAL;
  }
  markers[0].mtype = M_REFERENCE | M_TRACKING;
  markers[0].enabled = M_ENABLED;
  setting._active_marker = 0;
  set_sweep_points(POINTS_COUNT);
  dirty = true;
}

uint32_t calc_min_sweep_time_us(void)         // Estimate minimum sweep time in uS,  needed to calculate the initial delays for the RSSI before first sweep
{
  uint32_t t;
  if (MODE_OUTPUT(setting.mode))
    t = 200*sweep_points;                   // 200 microseconds is the delay set in perform when sweeping in output mode
  else {
    uint32_t bare_sweep_time=0;
#ifdef __SI4432__
    bare_sweep_time = (SI4432_step_delay + MEASURE_TIME) * (sweep_points); // Single RSSI delay and measurement time in uS while scanning
#endif
    if (FREQ_IS_CW()) {
      bare_sweep_time = MINIMUM_SWEEP_TIME;       // minimum sweep time in fast CW mode
      if (setting.repeat != 1 || setting.sweep_time_us >= 100*ONE_MS_TIME || S_STATE(setting.spur_removal)) // if no fast CW sweep possible
        bare_sweep_time = 15000;       // minimum CW sweep time when not in fast CW mode
    }
    t = vbwSteps * (S_STATE(setting.spur_removal) ? 2 : 1) * bare_sweep_time ;           // factor in vbwSteps and spur impact
    t += (setting.repeat - 1)* REPEAT_TIME * (sweep_points);            // Add time required for repeats
  }
  return t;
}


void set_refer_output(int v)
{
  setting.refer = v;
  set_calibration_freq(setting.refer);
//  dirty = true;
}

void set_decay(int d)
{
  if (d < 0 || d > 1000000)
    return;
  if (setting.frequency_step == 0) {        // decay in ms
    d = (float)d * 500.0 * (float)sweep_points / (float)setting.actual_sweep_time_us;
  }
  setting.decay = d;
  dirty = true;
}

#ifdef __QUASI_PEAK__
void set_attack(int d)
{
  if (d < 0 || d > 20000)
    return;
  if (setting.frequency_step == 0 && d>0) {        // decay in ms
    d = (float)d * 500.0 * (float)sweep_points  / (float)setting.actual_sweep_time_us;
  }
  setting.attack = d;
  dirty = true;
}
#endif

void set_noise(int d)
{
  if (d < 2 || d > 50)
    return;
  setting.noise = d;
  dirty = true;
}

void set_gridlines(int d)
{
  if (d < 3 || d > 20)
    return;
  config.gridlines = d;
  config_save();
  dirty = true;
  update_grid();
}

//int setting_frequency_10mhz = 10000000;

void set_10mhz(freq_t f)
{
  if (f < 9000000 || f > 11000000)
    return;
  config.setting_frequency_10mhz = f;
  config_save();
  dirty = true;
  update_grid();
}

void set_measurement(int m)
{
  setting.measurement = m;
#ifdef __LINEARITY__
  if (m == M_LINEARITY) {
    trace[TRACE_STORED].enabled = true;
    for (int j = 0; j < setting._sweep_points; j++)
      stored_t[j] = -150;
    setting.linearity_step = 0;
    setting.attenuate_x2 = 29*2;
    setting.auto_attenuation = false;
  }
#endif
  dirty = true;
}
void set_lo_drive(int d)
{
  setting.lo_drive = d;
  dirty = true;
}

void set_rx_drive(int d)
{
  setting.rx_drive = d;
  dirty = true;
}

void set_level_sweep(float l)
{
  setting.level_sweep = l;
  dirty = true;
}

void set_sweep_time_us(uint32_t t)          // Set the sweep time as the user wants it to be.
{
//  if (t < MINIMUM_SWEEP_TIME)             // Sweep time of zero means sweep as fast as possible
//    t = MINIMUM_SWEEP_TIME;
  if (t > MAXIMUM_SWEEP_TIME)
    t = MAXIMUM_SWEEP_TIME;
  setting.sweep_time_us = t;
//  if (MODE_OUTPUT(setting.mode))
//    setting.actual_sweep_time_us = t;       // To ensure time displayed is correct before first sweep is completed
#if 0
  uint32_t ta = calc_min_sweep_time_us();   // Can not be faster than minimum sweep time
  if (ta < t)
    ta = t;
  setting.actual_sweep_time_us = ta;
  if (FREQ_IS_CW())
    update_grid();            // Really only needed in zero span mode
  redraw_request |= REDRAW_FREQUENCY;
#endif
  dirty = true;
}

void set_tracking_output(int t)
{
  setting.tracking_output = t;
  dirty = true;
}

void toggle_tracking_output(void)
{
  setting.tracking_output = !setting.tracking_output;
  dirty = true;
}

void toggle_mirror_masking(void)
{
  setting.mirror_masking = !setting.mirror_masking;
  dirty = true;
}

void toggle_mute(void)
{
  setting.mute = !setting.mute;
  dirty = true;
}

void toggle_hambands(void)
{
  config.hambands = !config.hambands;
  dirty = true;
}

void toggle_below_IF(void)
{
  if (S_IS_AUTO(setting.below_IF ))
    setting.below_IF = false;
  else if (setting.below_IF)
    setting.below_IF = S_AUTO_OFF;
  else
    setting.below_IF = true;
  dirty = true;
}

void set_modulation(int m)
{
  setting.modulation = m;
  dirty = true;
}

void set_modulation_frequency(int f)
{
  if (50 <= f && f <= 6000) {
    setting.modulation_frequency = f;
    dirty = true;
  }
}

void set_repeat(int r)
{
  if (r > 0 && r <= 100) {
    setting.repeat = r;
//    dirty = true;             // No HW update required, only status panel refresh
  }
}

void set_IF(int f)
{
  if (f == 0)
    setting.auto_IF = true;
  setting.frequency_IF = f;
  dirty = true;
}

#ifdef TINYSA4
void set_IF2(int f)
{

  config.frequency_IF2 = f;
  dirty = true;
  config_save();
}

void set_R(int f)
{
  ADF4351_R_counter(f % 10);
  ADF4351_spur_mode(f/10);
  dirty = true;
}

void set_modulo(uint32_t f)
{
  ADF4350_modulo = f;
  //ADF4351_spur_mode(f);
  dirty = true;
}
#endif

#define POWER_STEP  0           // Should be 5 dB but appearently it is lower
#define POWER_OFFSET    15
#define SWITCH_ATTENUATION  30
#define RECEIVE_SWITCH_ATTENUATION  21


void set_auto_attenuation(void)
{
  setting.auto_attenuation = true;
  if (setting.mode == M_LOW) {
    setting.attenuate_x2 = 60;
  } else {
    setting.attenuate_x2 = 0;
  }
  setting.atten_step = false;
  dirty = true;
}

void set_auto_reflevel(int v)
{
  setting.auto_reflevel = v;
}

float get_attenuation(void)
{
  float actual_attenuation = setting.attenuate_x2 / 2.0;
  if (setting.mode == M_GENLOW) {
    if (setting.atten_step)
      return ( -(POWER_OFFSET + actual_attenuation - (setting.atten_step-1)*POWER_STEP + SWITCH_ATTENUATION));
    else
      return ( -POWER_OFFSET - actual_attenuation + (setting.rx_drive & 7) * 3);
  } else if (setting.atten_step) {
    if (setting.mode == M_LOW)
      return actual_attenuation + RECEIVE_SWITCH_ATTENUATION;
    else
      return actual_attenuation + SWITCH_ATTENUATION;
  }
  return(actual_attenuation);
}

static pureRSSI_t get_signal_path_loss(void){
#ifdef __ULTRA__
  if (setting.mode == M_ULTRA)
    return float_TO_PURE_RSSI(-15);       // Loss in dB, -9.5 for v0.1, -12.5 for v0.2
#endif
  if (setting.mode == M_LOW)
    return float_TO_PURE_RSSI(-5.5);      // Loss in dB, -9.5 for v0.1, -12.5 for v0.2
  return float_TO_PURE_RSSI(+7);          // Loss in dB (+ is gain)
}

static const int drive_dBm [16] = {-38,-35,-33,-30,-27,-24,-21,-19,-7,-4,-2, 1, 4, 7, 10, 13};

void set_level(float v)     // Set the output level in dB  in high/low output
{
  if (setting.mode == M_GENHIGH) {
    int d = 0;
    while (drive_dBm[d] < v - 1 && d < 15)          // Do not exceed array size
      d++;
    if (d == 8 && v < -12)  // Round towards closest level
      d = 7;
    set_lo_drive(d);
  } else {
    setting.level = v;
    set_attenuation((int)v);
  }
  dirty = true;
}

float get_level(void)
{
  if (setting.mode == M_GENHIGH) {
    return drive_dBm[setting.lo_drive];
  } else {
    return get_attenuation();
  }
}
void set_attenuation(float a)       // Is used both in low output mode and high/low input mode
{
  if (setting.mode == M_GENLOW) {
    a = a + POWER_OFFSET;
    if (a > 6) {                // +9dB
      setting.rx_drive = 11;   // Maximum save drive for SAW filters.
      a = a - 9;
    } else if (a > 3) {         // +6dB
      setting.rx_drive = 10;
      a = a - 6;
    } else if (a > 0) {         // +3dB
      setting.rx_drive = 9;
      a = a - 3;
    } else
      setting.rx_drive = 8;        // defined as 0dB level
    if (a > 0)
      a = 0;
    if( a >  - SWITCH_ATTENUATION) {
      setting.atten_step = 0;
    } else {
      a = a + SWITCH_ATTENUATION;
      setting.atten_step = 1;
    }
    a = -a;
  } else {
    if (setting.mode == M_LOW && a > 31) {
      setting.atten_step = 1;
      a = a - RECEIVE_SWITCH_ATTENUATION;
    } else if (setting.mode == M_HIGH && a > 0) {
      setting.atten_step = 1;
      a = a - SWITCH_ATTENUATION;
    } else
      setting.atten_step = 0;
    setting.auto_attenuation = false;
    dirty = true;
  }
  if (a<0.0)
      a = 0;
  if (a> 31)
    a=31.0;
  if (setting.mode == M_HIGH)   // No attenuator in high mode
    a = 0;
  if (setting.attenuate_x2 == a*2)
    return;
  setting.attenuate_x2 = a*2;
  dirty = true;
}

void set_storage(void)
{
  for (int i=0; i<POINTS_COUNT;i++)
    stored_t[i] = actual_t[i];
  setting.show_stored = true;
  trace[TRACE_STORED].enabled = true;
  //dirty = true;             // No HW update required, only status panel refresh
}

void set_clear_storage(void)
{
  setting.show_stored = false;
  setting.subtract_stored = false;
  trace[TRACE_STORED].enabled = false;
  // dirty = true;             // No HW update required, only status panel refresh
}

void set_subtract_storage(void)
{
  if (!setting.subtract_stored) {
    if (!setting.show_stored)
      set_storage();
    setting.subtract_stored = true;
    setting.normalize_level = 0.0;
//    setting.auto_attenuation = false;
  } else {
    setting.subtract_stored = false;
  }
  //dirty = true;             // No HW update required, only status panel refresh
}


void toggle_normalize(void)
{
  if (!setting.subtract_stored) {
    for (int i=0; i<POINTS_COUNT;i++)
      stored_t[i] = actual_t[i];
    setting.subtract_stored = true;
    setting.auto_attenuation = false;       // Otherwise noise level may move leading to strange measurements
    setting.normalize_level = 0.0;
  } else {
    setting.subtract_stored = false;
  }
  //dirty = true;             // No HW update required, only status panel refresh
}


extern float peakLevel;
void set_actual_power(float o)              // Set peak level to known value
{
  float new_offset = o - peakLevel + get_level_offset();        // calculate offset based on difference between measured peak level and known peak level
  if (o == 100) new_offset = 0;
  if (setting.mode == M_HIGH) {
    config.high_level_offset = new_offset;
  } else if (setting.mode == M_LOW) {
    config.low_level_offset = new_offset;
#ifdef __ULTRA__
  } else if (setting.mode == M_ULTRA) {
    config.low_level_offset = new_offset;
#endif
  }
  dirty = true;
  config_save();
  // dirty = true;             // No HW update required, only status panel refresh
}

float get_level_offset(void)
{
  if (setting.mode == M_HIGH) {
    if (config.high_level_offset == 100)        // Offset of 100 means not calibrated
      return 0;
    return(config.high_level_offset);
  }
  if (setting.mode == M_LOW) {
    if (config.low_level_offset == 100)
      return 0;
    return(config.low_level_offset);
  }
  return(0);
}

int level_is_calibrated(void)
{
  if (setting.mode == M_HIGH && config.high_level_offset != 100)
    return 1;
  if (setting.mode == M_LOW && config.low_level_offset != 100)
    return 1;
  return(0);
}

void set_RBW(uint32_t rbw_x10)
{
  setting.rbw_x10 = rbw_x10;
  update_rbw();
  dirty = true;
}

#ifdef __SPUR__
void set_spur(int v)
{
  if (setting.mode!=M_LOW)
    return;
  setting.spur_removal = v;
//  if (setting.spur_removal && actual_rbw > 360)           // moved to update_rbw
//    set_RBW(300);
  dirty = true;
}

void toggle_spur(void)
{
  if (setting.mode!=M_LOW)
    return;
#ifdef TINYSA4
  if (S_IS_AUTO(setting.spur_removal ))
    setting.spur_removal = false;
  else if (setting.spur_removal)
    setting.spur_removal = S_AUTO_OFF;
  else
    setting.spur_removal = true;
#else
  if (S_STATE(setting.spur_removal ))
    setting.spur_removal = S_OFF;
  else
    setting.spur_removal = S_ON;
#endif
  dirty = true;
}
#endif

#ifdef __ULTRA__
void set_harmonic(int h)
{
  setting.harmonic = h;
  minFreq = 684000000.0;
  if ((freq_t)(setting.harmonic * 240000000)+434000000 >  minFreq)
    minFreq = setting.harmonic * 240000000.0+434000000.0;
  maxFreq = 4360000000;
  if (setting.harmonic != 0 && (960000000.0 * setting.harmonic + 434000000.0 )< 4360000000.0)
    maxFreq = (960000000.0 * setting.harmonic + 434000000.0 );
  set_sweep_frequency(ST_START, minFreq);
  set_sweep_frequency(ST_STOP, maxFreq);
}
#endif

void set_step_delay(int d)                  // override RSSI measurement delay or set to one of three auto modes
{

  if ((3 <= d && d < 100) || d > 30000)         // values 0 (normal scan), 1 (precise scan) and 2(fast scan) have special meaning and are auto calculated
    return;
  if (d <3) {
    setting.step_delay_mode = d;
    setting.step_delay = 0;
    setting.offset_delay = 0;
  } else {
    setting.step_delay_mode = SD_MANUAL;
    setting.step_delay = d;
  }
  dirty = true;
}

void set_offset_delay(int d)                  // override RSSI measurement delay or set to one of three auto modes
{
 setting.offset_delay = d;
 dirty = true;
}


void set_average(int v)
{
  setting.average = v;
  trace[TRACE_TEMP].enabled = ((v != 0)
#ifdef __QUASI_PEAK__
      && (v != AV_QUASI)
#endif
      );
  //dirty = true;             // No HW update required, only status panel refresh
}

void toggle_LNA(void)
{
  if (S_IS_AUTO(setting.lna ))
    setting.lna = false;
  else if (setting.lna)
    setting.lna = S_AUTO_OFF;
  else
    setting.lna = true;
  dirty = true;
}

void toggle_tracking(void)
{
  setting.tracking = !setting.tracking;
  if (setting.tracking) {
    set_refer_output(2);
    set_sweep_frequency(ST_CENTER, 10000000);
    set_sweep_frequency(ST_SPAN,    5000000);
  } else {
    set_refer_output(-1);
  }
  dirty = true;
}

void toggle_AGC(void)
{
  if (S_IS_AUTO(setting.agc ))
    setting.agc = false;
  else if (setting.agc)
    setting.agc = S_AUTO_ON;
  else
    setting.agc = true;
  dirty = true;
}

static unsigned char SI4432_old_v[2];

void auto_set_AGC_LNA(int auto_set, int agc)                                                                    // Adapt the AGC setting if needed
{
#ifdef __SI4432__
  unsigned char v;
  if (auto_set)
    v = 0x60; // Enable AGC and disable LNA
  else
    v = 0x40+agc; // Disable AGC and enable LNA
  if (SI4432_old_v[MODE_SELECT(setting.mode)] != v) {
    SI4432_Sel = MODE_SELECT(setting.mode);
    SI4432_Write_Byte(SI4432_AGC_OVERRIDE, v);
    SI4432_old_v[MODE_SELECT(setting.mode)] = v;
  }
#endif
}

#ifdef __SI4432__
void set_AGC_LNA(void) {
  unsigned char v = 0x40;
  if (S_STATE(setting.agc)) v |= 0x20;
  if (S_STATE(setting.lna)) v |= 0x10;
  SI4432_Write_Byte(SI4432_AGC_OVERRIDE, v);
  SI4432_old_v[MODE_SELECT(setting.mode)] = v;
}
#endif

void set_unit(int u)
{
  if (setting.unit == u)
    return;
  float r = to_dBm(setting.reflevel);   // Get neutral unit
  float s = to_dBm(setting.scale);
//  float t = setting.trigger;            // Is always in dBm
  // float m = r - NGRIDSY*s;

  setting.unit = u;                     // Switch unit

  r = value(r);                         // Convert to target unit
  s = value(s);
  if (UNIT_IS_LINEAR(setting.unit)) {
    if (r < REFLEVEL_MIN)
      r = REFLEVEL_MIN;                          // Minimum value to ensure display
    if (r >REFLEVEL_MAX)
      r = REFLEVEL_MAX;                          // Maximum value
    set_scale(r/NGRIDY);
    set_reflevel(setting.scale*NGRIDY);
    if (S_IS_AUTO(setting.agc))
      setting.agc = S_AUTO_ON;
    if (S_IS_AUTO(setting.lna))
      setting.lna = S_AUTO_OFF;
  } else {
    r = 10 * round((r*1.2)/10.0);
    set_reflevel(r);
    set_scale(10);
    if (S_IS_AUTO(setting.agc))
      setting.agc = S_AUTO_ON;
    if (S_IS_AUTO(setting.lna))
      setting.lna = S_AUTO_OFF;
  }
  plot_into_index(measured);
  redraw_request|=REDRAW_AREA;
  //dirty = true;             // No HW update required, only status panel refresh
}
float const unit_scale_value[]={1,0.001,0.000001,0.000000001,0.000000000001};
const char * const unit_scale_text[]= {"","m", "\035",     "n",        "p"};

void user_set_reflevel(float level)
{
  set_auto_reflevel(false);
  if (UNIT_IS_LINEAR(setting.unit) && level < setting.scale*NGRIDY) {
    set_scale(level/NGRIDY);
    set_reflevel(setting.scale*NGRIDY);
  } else
    set_reflevel(level);
  redraw_request|=REDRAW_AREA;
}

void set_reflevel(float level)
{

  if (UNIT_IS_LINEAR(setting.unit)) {
    if (level < REFLEVEL_MIN)
      level = REFLEVEL_MIN;
    if (level > REFLEVEL_MAX)
      level = REFLEVEL_MAX;
  }

  setting.unit_scale_index = 0;
  setting.unit_scale = 1.0;
  while (UNIT_IS_LINEAR(setting.unit) && setting.unit_scale_index < sizeof(unit_scale_value)/sizeof(float) - 1) {
    if (level > unit_scale_value[setting.unit_scale_index])
      break;
    setting.unit_scale_index++;
  }
  setting.unit_scale = unit_scale_value[setting.unit_scale_index];
  setting.reflevel = level;
  set_trace_refpos(level);
//  dirty = true;
}

void round_reflevel_to_scale(void) {
  int multi = floor((setting.reflevel + setting.scale/2)/setting.scale);
  if (UNIT_IS_LINEAR(setting.unit)) {
    if (multi < NGRIDY) {
      setting.reflevel = setting.scale*10;  // Never negative bottom
    }
  } else {

  }
  setting.reflevel = multi*setting.scale;
  set_trace_refpos(setting.reflevel);
}

void user_set_scale(float s)
{
  if (UNIT_IS_LINEAR(setting.unit))
    set_auto_reflevel(false);
  set_scale(s);
  if (UNIT_IS_LINEAR(setting.unit) && setting.reflevel < setting.scale*NGRIDY)
    set_reflevel(setting.scale*NGRIDY);
  force_set_markmap();
}

void set_scale(float t) {
  if (UNIT_IS_LINEAR(setting.unit)) {
    if (t < REFLEVEL_MIN/10.0)
      t = REFLEVEL_MIN/10.0;
    if (t > REFLEVEL_MAX/10.0)
      t = REFLEVEL_MAX/10.0;
  } else {
    if (t > 20.0)
      t = 20.0;
    else if (t < 1)
      t = 1.0;
  }

  float m = 1;
//        t = t * 1.2;
  while (t > 10) { m *= 10; t/=10; }
  while (t < 1.0)  { m /= 10; t*=10; }
  if (t>5.0001)
    t = 10.0;
  else if (t>2.0001)
    t = 5.0;
  else if (t > 1.0001)
    t = 2.0;
  else
    t = 1.0;
  t = t*m;
  setting.scale = t;
  set_trace_scale(t);
  round_reflevel_to_scale();
}

extern char low_level_help_text[12];

void set_offset(float offset)
{
  setting.offset = offset;
  int min,max;
  if (setting.mode == M_GENLOW) {
    min = -76; max = -6;
  } else {
    min = -38; max = +13;
  }
  plot_printf(low_level_help_text, sizeof low_level_help_text, "%+d..%+d", min + (int)offset, max + (int)offset);
  force_set_markmap();
  dirty = true;             // No HW update required, only status panel refresh but need to ensure the cached value is updated in the calculation of the RSSI
}

void set_trigger_level(float trigger_level)
{
  setting.trigger_level = trigger_level;
  redraw_request |= REDRAW_TRIGGER | REDRAW_CAL_STATUS | REDRAW_AREA;
  //dirty = true;             // No HW update required, only status panel refresh
}

void set_trigger(int trigger)
{
  if (trigger == T_PRE || trigger == T_POST || trigger == T_MID) {
    setting.trigger_mode = trigger;
  } else if (trigger == T_UP || trigger == T_DOWN){
    setting.trigger_direction = trigger;
  } else if (trigger == T_DONE) {
    pause_sweep();                    // Trigger once so pause after this sweep has completed!!!!!!!
    redraw_request |= REDRAW_CAL_STATUS;        // Show status change    setting.trigger = trigger;
    setting.trigger = trigger;
  } else {
    sweep_mode = SWEEP_ENABLE;
    setting.trigger = trigger;
  }
  redraw_request|=REDRAW_TRIGGER | REDRAW_CAL_STATUS;
  //dirty = true;             // No HW update required, only status panel refresh
}


//int GetRefpos(void) {
//  return (NGRIDY - get_trace_refpos(2)) * get_trace_scale(2);
//}

//int GetScale(void) {
//  return get_trace_refpos(2);
//}
void set_mode(int m)
{
#ifdef __ULTRA__
  if (m == 6)
    m = M_ULTRA;
#endif
  dirty = true;
  if (setting.mode == m)
    return;
  reset_settings(m);
//  dirty = true;
}

void set_fast_speedup(int s)
{
  setting.fast_speedup = s;
  dirty = true;
}

void calculate_step_delay(void)
{
  if (setting.step_delay_mode == SD_MANUAL || setting.step_delay != 0) {        // The latter part required for selftest 3
    SI4432_step_delay = setting.step_delay;
    if (setting.offset_delay != 0)      // Override if set
      SI4432_offset_delay = setting.offset_delay;
  } else {
    SI4432_offset_delay = 0;
    if (setting.frequency_step == 0) {            // zero span mode, not dependent on selected RBW
      SI4432_step_delay = 0;
    } else {
#ifdef __SI4432__
#if 1       // Table for double offset delay
      if (actual_rbw_x10 >= 1910)      { SI4432_step_delay =  300; SI4432_offset_delay = 100; }
      else if (actual_rbw_x10 >= 1420) { SI4432_step_delay =  350; SI4432_offset_delay = 100; }
      else if (actual_rbw_x10 >= 750)  { SI4432_step_delay =  450; SI4432_offset_delay = 100; }
      else if (actual_rbw_x10 >= 560)  { SI4432_step_delay =  650; SI4432_offset_delay = 100; }
      else if (actual_rbw_x10 >= 370)  { SI4432_step_delay =  700; SI4432_offset_delay = 200; }
      else if (actual_rbw_x10 >= 180)  { SI4432_step_delay = 1100; SI4432_offset_delay = 300; }
      else if (actual_rbw_x10 >=  90)  { SI4432_step_delay = 1700; SI4432_offset_delay = 400; }
      else if (actual_rbw_x10 >=  50)  { SI4432_step_delay = 3300; SI4432_offset_delay = 800; }
      else                             { SI4432_step_delay = 6400; SI4432_offset_delay =1600; }
#else
      if (actual_rbw_x10 >= 1910)      { SI4432_step_delay =  280; SI4432_offset_delay = 100; }
      else if (actual_rbw_x10 >= 1420) { SI4432_step_delay =  350; SI4432_offset_delay = 100; }
      else if (actual_rbw_x10 >= 750)  { SI4432_step_delay =  450; SI4432_offset_delay = 100; }
      else if (actual_rbw_x10 >= 560)  { SI4432_step_delay =  650; SI4432_offset_delay = 100; }
      else if (actual_rbw_x10 >= 370)  { SI4432_step_delay =  700; SI4432_offset_delay = 100; }
      else if (actual_rbw_x10 >= 180)  { SI4432_step_delay = 1100; SI4432_offset_delay = 200; }
      else if (actual_rbw_x10 >=  90)  { SI4432_step_delay = 1700; SI4432_offset_delay = 400; }
      else if (actual_rbw_x10 >=  50)  { SI4432_step_delay = 3300; SI4432_offset_delay = 400; }
      else                             { SI4432_step_delay = 6400; SI4432_offset_delay =1600; }
#endif
#endif
      if (setting.step_delay_mode == SD_PRECISE)    // In precise mode wait twice as long for RSSI to stabalize
        SI4432_step_delay *= 2;
      if (setting.fast_speedup >0)
        SI4432_offset_delay = SI4432_step_delay / setting.fast_speedup;
    }
    if (setting.offset_delay != 0)      // Override if set
      SI4432_offset_delay = setting.offset_delay;
  }
}

void apply_settings(void)       // Ensure all settings in the setting structure are translated to the right HW setup
{
  set_switches(setting.mode);
#ifdef __PE4302__
  if (setting.mode == M_HIGH)
    PE4302_Write_Byte(40);  // Ensure defined input impedance of low port when using high input mode (power calibration)
  else
    PE4302_Write_Byte((int)(setting.attenuate_x2));
#endif
  if (setting.mode == M_LOW) {

  }
  set_calibration_freq(setting.refer);
  update_rbw();
  calculate_step_delay();
}

//------------------------------------------
#if 0
#define CORRECTION_POINTS  10

static const freq_t correction_frequency[CORRECTION_POINTS] =
{ 100000, 200000, 400000, 1000000, 2000000, 50000000, 100000000, 200000000, 300000000, 350000000 };

static const float correction_value[CORRECTION_POINTS] =
{ +4.0, +2.0, +1.5, +0.5, 0.0, 0.0, +1.0, +1.0, +2.5, +5.0 };
#endif

/*
 * To avoid float calculations the correction values are maximum +/-16 and accuracy of 0.5 so they fit easily in 8 bits
 * The frequency steps between correction factors is assumed to be maximum 500MHz or 0x2000000 and minimum 100kHz or > 0x10000
 * The divider 1/m is pre-calculated into delta_div as 2^scale_factor * correction_step/frequency_step
 */

#define SCALE_FACTOR 14     // min scaled correction = 2^15, max scaled correction = 256 * 2^15
                            // min scaled f = 6, max scaled f =  1024

static int32_t scaled_correction_multi[CORRECTION_POINTS];
static int32_t scaled_correction_value[CORRECTION_POINTS];

void calculate_correction(void)
{
  scaled_correction_value[0] = setting.correction_value[0]  * (1 << (SCALE_FACTOR));
  for (int i = 1; i < CORRECTION_POINTS; i++) {
    scaled_correction_value[i] = setting.correction_value[i]  * (1 << (SCALE_FACTOR));
    int32_t m = scaled_correction_value[i] - scaled_correction_value[i-1];
    int32_t d = (setting.correction_frequency[i] - setting.correction_frequency[i-1]) >> SCALE_FACTOR;
    scaled_correction_multi[i] = (int32_t) ( m / d );
  }
}
#pragma GCC push_options
#pragma GCC optimize ("Og")             // "Os" causes problem

pureRSSI_t get_frequency_correction(freq_t f)      // Frequency dependent RSSI correction to compensate for imperfect LPF
{
  if (setting.mode == M_GENHIGH)
    return(0.0);
  int i = 0;
  while (f > setting.correction_frequency[i] && i < CORRECTION_POINTS)
    i++;
  if (i >= CORRECTION_POINTS)
    return(scaled_correction_value[CORRECTION_POINTS-1] >> (SCALE_FACTOR - 5) );
  if (i == 0)
    return(scaled_correction_value[0] >> (SCALE_FACTOR - 5) );
  f = f - setting.correction_frequency[i-1];
#if 0
  freq_t m = (setting.correction_frequency[i] - setting.correction_frequency[i-1]) >> SCALE_FACTOR ;
  float multi = (setting.correction_value[i] - setting.correction_value[i-1]) * (1 << (SCALE_FACTOR -1)) / (float)m;
  float cv = setting.correction_value[i-1] + ((f >> SCALE_FACTOR) * multi) / (float)(1 << (SCALE_FACTOR -1)) ;
#else
  int32_t scaled_f = f >> SCALE_FACTOR;
  pureRSSI_t cv = (scaled_correction_value[i-1] + (scaled_f * scaled_correction_multi[i])) >> (SCALE_FACTOR - 5) ;
#endif
  return(cv);
}
#pragma GCC pop_options



float peakLevel;
float min_level;
freq_t peakFreq;
int peakIndex;
float temppeakLevel;
int temppeakIndex;

// volatile int t;

void setupSA(void)
{
#ifdef __SI4432__
  SI4432_Init();
#endif
  for (unsigned int i = 0; i < sizeof(old_freq)/sizeof(unsigned long) ; i++) {
    old_freq[i] = 0;
    real_old_freq[i] = 0;
  }
#ifdef __SI4432__
  SI4432_Sel = SI4432_RX ;
  SI4432_Receive();

  SI4432_Sel = SI4432_LO ;
  SI4432_Transmit(0);
#endif
#ifdef __PE4302__
  PE4302_init();
  PE4302_Write_Byte(0);
#endif
#if 0           // Measure fast scan time
  setting.sweep_time_us = 0;
  setting.additional_step_delay_us = 0;
  START_PROFILE             // measure 90 points to get overhead
  SI4432_Fill(0,200);
  int t1 = DELTA_TIME;
  RESTART_PROFILE           // measure 290 points to get real added time for 200 points
  SI4432_Fill(0,0);
  int t2 = DELTA_TIME;
  int t = (t2 - t1) * 100 * (sweep_points) / 200; // And calculate real time excluding overhead for all points
#endif
}

#define __WIDE_OFFSET__
#ifdef __WIDE_OFFSET__
#define OFFSET_LOWER_BOUND -80000
#else
#define OFFSET_LOWER_BOUND 0
#endif

void set_freq(int V, unsigned long freq)    // translate the requested frequency into a setting of the SI4432
{
  if (old_freq[V] == freq)       // Do not change HW if not needed
    return;
  if (V <= 1) {
#ifdef __SI4432__
    SI4432_Sel = V;
    if (freq < 240000000 || freq > 960000000) {   // Impossible frequency, simply ignore, should never happen.
      real_old_freq[V] = freq + 1; // No idea why this is done........
      return;
    }
#if 1
    if (V == 1 && setting.step_delay_mode == SD_FAST) {        // If in extra fast scanning mode and NOT SI4432_RX !!!!!!
      int delta =  freq - real_old_freq[V];

      if (real_old_freq[V] >= 480000000)    // 480MHz, high band
        delta = delta >> 1;
      if (delta > OFFSET_LOWER_BOUND && delta < 79999) { // and requested frequency can be reached by using the offset registers
#if 0
        if (real_old_freq[V] >= 480000000)
          shell_printf("%d: Offs %q HW %d\r\n", SI4432_Sel, (freq_t)(real_old_freq[V]+delta*2),  real_old_freq[V]);
        else
          shell_printf("%d: Offs %q HW %d\r\n", SI4432_Sel, (freq_t)(real_old_freq[V]+delta*1),  real_old_freq[V]);
#endif
        delta = delta * 4 / 625; // = 156.25;             // Calculate and set the offset register i.s.o programming a new frequency
        SI4432_Write_2_Byte(SI4432_FREQ_OFFSET1, (uint8_t)(delta & 0xff), (uint8_t)((delta >> 8) & 0x03));
 //       SI4432_Write_Byte(SI4432_FREQ_OFFSET2, (uint8_t)((delta >> 8) & 0x03));
        SI4432_offset_changed = true;                 // Signal offset changed so RSSI retrieval is delayed for frequency settling
        old_freq[V] = freq;
      } else {
#ifdef __WIDE_OFFSET__
        freq_t target_f;                    // Impossible to use offset so set SI4432 to new frequency
        if (freq < real_old_freq[V]) {                          // sweeping down
          if (freq - 80000 >= 480000000) {
            target_f = freq - 160000;
          } else {
            target_f = freq - 80000;
          }
          SI4432_Set_Frequency(target_f);
          SI4432_Write_2_Byte(SI4432_FREQ_OFFSET1, 0xff, 0x01);           // set offset to most positive
 //         SI4432_Write_Byte(SI4432_FREQ_OFFSET2, 0x01);
          real_old_freq[V] = target_f;
        } else {                                                // sweeping up
          if (freq + 80000 >= 480000000) {
            target_f = freq + 160000;
          } else {
            target_f = freq + 80000;
          }
          SI4432_Set_Frequency(target_f);
          SI4432_Write_2_Byte(SI4432_FREQ_OFFSET1, 0, 0x02);           // set offset to most negative
//          SI4432_Write_Byte(SI4432_FREQ_OFFSET2, 0x02);
          real_old_freq[V] = target_f;
        }
#else
        SI4432_Set_Frequency(freq);           // Impossible to use offset so set SI4432 to new frequency
        SI4432_Write_2_Byte(SI4432_FREQ_OFFSET1, 0, 0);           // set offset to zero
//        SI4432_Write_Byte(SI4432_FREQ_OFFSET2, 0);
        real_old_freq[V] = freq;
#endif
      }
    } else {
#endif
      SI4432_Set_Frequency(freq);           // Not in fast mode
      real_old_freq[V] = freq;
    }
#endif
  }
  old_freq[V] = freq;
}

#ifdef __SI4432__
void set_switch_transmit(void) {
  SI4432_Write_Byte(SI4432_GPIO0_CONF, 0x1f);// Set switch to transmit
  SI4432_Write_Byte(SI4432_GPIO1_CONF, 0x1d);
}

void set_switch_receive(void) {
  SI4432_Write_Byte(SI4432_GPIO0_CONF, 0x1d);// Set switch to receive
  SI4432_Write_Byte(SI4432_GPIO1_CONF, 0x1f);
}

void set_switch_off(void) {
  SI4432_Write_Byte(SI4432_GPIO0_CONF, 0x1d);// Set both switch off
  SI4432_Write_Byte(SI4432_GPIO1_CONF, 0x1f);
}

#endif

void set_switches(int m)
{
#ifdef __SI4432__
  SI4432_Init();
  old_freq[0] = 0;
  old_freq[1] = 0;
  real_old_freq[0] = 0;
  real_old_freq[1] = 0;
  SI4432_Sel = SI4432_LO ;
  SI4432_Write_Byte(SI4432_FREQ_OFFSET1, 0);  // Back to nominal offset
  SI4432_Write_Byte(SI4432_FREQ_OFFSET2, 0);
#endif
  switch(m) {
case M_LOW:     // Mixed into 0
#ifdef __ULTRA__
case M_ULTRA:
#endif
#ifdef __SI4432__
    SI4432_Sel = SI4432_RX ;
    SI4432_Receive();
    if (setting.atten_step) {   // use switch as attenuator
      set_switch_transmit();
    } else {
      set_switch_receive();
    }
#endif
    set_AGC_LNA();

#ifdef __SI4432__
    SI4432_Sel = SI4432_LO ;
    if (setting.tracking_output)
      set_switch_transmit();
    else
      set_switch_off();
//    SI4432_Receive(); For noise testing only
    SI4432_Transmit(setting.lo_drive);
    // set_calibration_freq(setting.refer);
#endif
    break;
case M_HIGH:    // Direct into 1
mute:
#ifdef __SI4432__
    // set_calibration_freq(-1); // Stop reference output
    SI4432_Sel = SI4432_RX ; // both as receiver to avoid spurs
    set_switch_receive();
    SI4432_Receive();

    SI4432_Sel = SI4432_LO ;
    SI4432_Receive();
    if (setting.atten_step) {// use switch as attenuator
       set_switch_transmit();
     } else {
       set_switch_receive();
     }
#endif
    set_AGC_LNA();

    break;
case M_GENLOW:  // Mixed output from 0
    if (setting.mute)
      goto mute;
#ifdef __SI4432__
    SI4432_Sel = SI4432_RX ;
    if (setting.atten_step) { // use switch as attenuator
      set_switch_off();
    } else {
      set_switch_transmit();
    }
    SI4432_Transmit(setting.rx_drive);

    SI4432_Sel = SI4432_LO ;
    if (setting.modulation == MO_EXTERNAL) {
      set_switch_transmit();  // High input for external LO scuh as tracking output of other tinySA
      SI4432_Receive();
    } else {
      set_switch_off();
      SI4432_Transmit(12);                 // Fix LO drive a 10dBm
    }
#endif
    break;
case M_GENHIGH: // Direct output from 1
    if (setting.mute)
      goto mute;
#ifdef __SI4432__
    SI4432_Sel = SI4432_RX ;
    SI4432_Receive();
    set_switch_receive();

    SI4432_Sel = SI4432_LO ;
    if (setting.lo_drive < 8) {
      set_switch_off(); // use switch as attenuator
    } else {
      set_switch_transmit();
    }
    SI4432_Transmit(setting.lo_drive);
#endif
    break;
  }

}

void update_rbw(void)           // calculate the actual_rbw and the vbwSteps (# steps in between needed if frequency step is largen than maximum rbw)
{
  if (setting.frequency_step > 0 && MODE_INPUT(setting.mode)) {
    setting.vbw_x10 = (setting.frequency_step)/100;
  } else {
    setting.vbw_x10 = 3000; // trick to get right default rbw in zero span mode
  }
  freq_t temp_actual_rbw_x10 = setting.rbw_x10;     // requested rbw , 32 bit !!!!!!
  if (temp_actual_rbw_x10 == 0) {        // if auto rbw
    if (setting.step_delay_mode==SD_FAST) {    // if in fast scanning
      if (setting.fast_speedup > 2)
        temp_actual_rbw_x10 = 6*setting.vbw_x10; // rbw is six times the frequency step to ensure no gaps in coverage as there are some weird jumps
      else
        temp_actual_rbw_x10 = 4*setting.vbw_x10; // rbw is four times the frequency step to ensure no gaps in coverage as there are some weird jumps
    } else
      temp_actual_rbw_x10 = 2*setting.vbw_x10; // rbw is twice the frequency step to ensure no gaps in coverage
  }
#ifdef __SI4432__
  if (temp_actual_rbw_x10 < 26)
    temp_actual_rbw_x10 = 26;
  if (temp_actual_rbw_x10 > 6000)
    temp_actual_rbw_x10 = 6000;
#endif
  actual_rbw_x10 = temp_actual_rbw_x10;         // Now it fits in 16 bit
#ifdef __SI4432__
  if (S_STATE(setting.spur_removal) && actual_rbw_x10 > 3000)
    actual_rbw_x10 = 2500;           // if spur suppression reduce max rbw to fit within BPF
  SI4432_Sel =  MODE_SELECT(setting.mode);
#endif
  actual_rbw_x10 = set_rbw(actual_rbw_x10);  // see what rbw the SI4432 can realize
  if (setting.frequency_step > 0 && MODE_INPUT(setting.mode)) { // When doing frequency scanning in input mode
    vbwSteps = ((int)(2 * (setting.vbw_x10 + (actual_rbw_x10/2)) / actual_rbw_x10)); // calculate # steps in between each frequency step due to rbw being less than frequency step
    if (setting.step_delay_mode==SD_PRECISE)    // if in Precise scanning
      vbwSteps *= 2;                            // use twice as many steps
    if (vbwSteps < 1)                            // at least one step
      vbwSteps = 1;
  } else {                      // in all other modes
    setting.vbw_x10 = actual_rbw_x10;
    vbwSteps = 1;               // only one vbwSteps
  }
}

int binary_search_frequency(int f)      // Search which index in the frequency tabled matches with frequency  f using actual_rbw
{
  int L = 0;
  int R =  (sizeof frequencies)/sizeof(int) - 1;
  int fmin =  f - actual_rbw_x10 * 100;
  int fplus = f + actual_rbw_x10 * 100;
  while (L <= R) {
    int m = (L + R) / 2;
    if ((int)frequencies[m] < fmin)
      L = m + 1;
    else if ((int)frequencies[m] > fplus)
      R = m - 1;
    else
       return m; // index is m
  }
  return -1;
}

void interpolate_maximum(int m)
{
  const int idx          = markers[m].index;
  markers[m].frequency = frequencies[idx];
  if (idx > 0 && idx < sweep_points-1)
  {
    const float y1         = actual_t[idx - 1];
    const float y2         = actual_t[idx + 0];
    const float y3         = actual_t[idx + 1];
    const float d          = 0.5f * (y1 - y3) / ((y1 - (2 * y2) + y3) + 1e-12f);
    //const float bin      = (float)idx + d;
    const int32_t delta_Hz = abs((int64_t)frequencies[idx + 0] - frequencies[idx + 1]);
    markers[m].frequency   += (int32_t)(delta_Hz * d);
  }
}

#define MAX_MAX 4
int
search_maximum(int m, int center, int span)
{
  center = binary_search_frequency(center);
  if (center < 0)
    return false;
  int from = center - span/2;
  int found = false;
  int to = center + span/2;
  int cur_max = 0;          // Always at least one maximum
  int max_index[4];
  if (from<0)
    from = 0;
  if (to > setting._sweep_points-1)
    to = setting._sweep_points-1;
  temppeakIndex = 0;
  temppeakLevel = actual_t[from];
  max_index[cur_max] = from;
  int downslope = true;

  for (int i = from; i <= to; i++) {
    if (downslope) {
      if (temppeakLevel > actual_t[i]) {    // Follow down
        temppeakIndex = i;                  // Latest minimum
        temppeakLevel = actual_t[i];
      } else if (temppeakLevel + setting.noise < actual_t[i]) {    // Local minimum found
        temppeakIndex = i;                          // This is now the latest maximum
        temppeakLevel = actual_t[i];
        downslope = false;
      }
    } else {
      if (temppeakLevel < actual_t[i]) {    // Follow up
        temppeakIndex = i;
        temppeakLevel = actual_t[i];
      } else if (temppeakLevel - setting.noise > actual_t[i]) {    // Local max found

        found = true;
        int j = 0;                                            // Insertion index
        while (j<cur_max && actual_t[max_index[j]] >= temppeakLevel)   // Find where to insert
          j++;
        if (j < MAX_MAX) {                                    // Larger then one of the previous found
          int k = MAX_MAX-1;
          while (k > j) {                                      // Shift to make room for max
            max_index[k] = max_index[k-1];
            //              maxlevel_index[k] = maxlevel_index[k-1];        // Only for debugging
            k--;
          }
          max_index[j] = temppeakIndex;
          //            maxlevel_index[j] = actual_t[temppeakIndex];      // Only for debugging
          if (cur_max < MAX_MAX) {
            cur_max++;
          }
          //STOP_PROFILE
        }
        temppeakIndex = i;            // Latest minimum
        temppeakLevel = actual_t[i];

        downslope = true;
      }
    }
  }
  markers[m].index = max_index[0];
  interpolate_maximum(m);
//  markers[m].frequency = frequencies[markers[m].index];
  return found;
}

//static int spur_old_stepdelay = 0;
static const unsigned int spur_IF =            DEFAULT_IF;       // The IF frequency for which the spur table is value
static const unsigned int spur_alternate_IF =  DEFAULT_SPUR_IF;       // if the frequency is found in the spur table use this IF frequency
static const int spur_table[] =                                 // Frequencies to avoid
{
// 580000,            // 433.8 MHz table
// 880000,    //?
 960000,
// 1487000,   //?
 1600000,
// 1837000,           // Real signal
// 2755000,           // Real signal
// 2760000,
 2960000,
 4933000,
 4960000,
 6960000,
// 6980000,
 8267000,
 8960000,
// 10000000,
 10960000,
 11600000,
 12960000,
 14933000,
 14960000,
 16960000,
 18960000,
 21600000,

// 22960000,
 24960000,
 28960000,
// 29800000,
 31600000,
 34960000,
 33930000,
// 38105000,
 40960000,
 41600000,
 49650000,
 272400000,
 287950000,
// 288029520,
 332494215,
};

int binary_search(int f)
{
  int L = 0;
  int R =  (sizeof spur_table)/sizeof(int) - 1;
  int fmin =  f - actual_rbw_x10 * (100 / 2);
  int fplus = f + actual_rbw_x10 * (100 / 2);
  while (L <= R) {
    int m = (L + R) / 2;
    if (spur_table[m] < fmin)
      L = m + 1;
    else if (spur_table[m] > fplus)
      R = m - 1;
    else
       return true; // index is m
  }
  return false;
}


int avoid_spur(int f)                   // find if this frequency should be avoided
{
//  int window = ((int)actual_rbw ) * 1000*2;
//  if (window < 50000)
//    window = 50000;
  if (setting.mode != M_LOW || !setting.auto_IF || actual_rbw_x10 > 3000)
    return(false);
  return binary_search(f);
}

static int modulation_counter = 0;

#define MODULATION_STEPS    8
static const int am_modulation[MODULATION_STEPS] =  { 5, 1, 0, 1, 5, 9, 11, 9 };         // AM modulation
//
//  Offset is 156.25Hz when below 600MHz and 312.5 when above.
//
#define LND  16   // Total NFM deviation is LND * 4 * 156.25 = 5kHz when below 600MHz or 600MHz - 434MHz
#define HND  8
#define LWD  96 // Total WFM deviation is LWD * 4 * 156.25 = 30kHz when below 600MHz
#define HWD  48
static const int fm_modulation[4][MODULATION_STEPS] =  // Avoid sign changes in NFM
{
 { 2*LND,(int)( 3.5*LND ), 4*LND, (int)(3.5*LND), 2*LND, (int)(0.5*LND), 0, (int)(0.5*LND)},                // Low range, NFM
 { 0*LWD,(int)( 1.5*LWD ), 2*LWD, (int)(1.5*LWD), 0*LWD, (int)(-1.5*LWD), (int)-2*LWD, (int)(-1.5*LWD)},    // Low range, WFM
 { 2*HND,(int)( 3.5*HND ), 4*HND, (int)(3.5*HND), 2*HND, (int)(0.5*HND), 0, (int)(0.5*HND)},                // High range, NFM
 { 0*HWD,(int)( 1.5*HWD ), 2*HWD, (int)(1.5*HWD), 0*HWD, (int)(-1.5*HWD), (int)-2*HWD, (int)(-1.5*HWD)},    // HIgh range, WFM
};    // narrow FM modulation avoid sign changes

static const int fm_modulation_offset[4] =
{
   85000,
   80000,
  165000,
  160000
};


deviceRSSI_t age[POINTS_COUNT];     // Array used for 1: calculating the age of any max and 2: buffer for fast sweep RSSI values;

static float old_a = -150;          // cached value to reduce writes to level registers
static pureRSSI_t correct_RSSI;
static pureRSSI_t correct_RSSI_freq;
systime_t start_of_sweep_timestamp;
static systime_t sweep_elapsed = 0;                             // Time since first start of sweeping, used only for auto attenuate
uint8_t signal_is_AM = false;
static uint8_t check_for_AM = false;
static int is_below = false;

static void calculate_static_correction(void)                   // Calculate the static part of the RSSI correction
{
  correct_RSSI =
#ifdef __SI4432__
      getSI4432_RSSI_correction()
#endif
      - get_signal_path_loss()
      + float_TO_PURE_RSSI(
          + get_level_offset()
          + get_attenuation()
          - setting.offset);
}

int hsical = -1;
void clock_above_48MHz(void)
{
  if (hsical == -1)
    hsical = (RCC->CR & 0xff00) >> 8;
  if (hsical != -1) {
    RCC->CR &= RCC_CR_HSICAL;
    RCC->CR |= ( (hsical) << 8 );
    RCC->CR &= RCC_CR_HSITRIM | RCC_CR_HSION; /* CR Reset value.              */
    RCC->CR |= RCC_CR_HSITRIM_4 | RCC_CR_HSITRIM_0 | RCC_CR_HSITRIM_1;
  }
}

void clock_below_48MHz(void)
{
  if (hsical == -1)
    hsical = ( (RCC->CR & 0xff00) >> 8 );
  if (hsical != -1) {
    RCC->CR &= RCC_CR_HSICAL;
    RCC->CR |= ( (hsical) << 8 );
    RCC->CR &= RCC_CR_HSITRIM | RCC_CR_HSION; /* CR Reset value.              */
    RCC->CR |=  RCC_CR_HSITRIM_2 | RCC_CR_HSITRIM_3;
  }
}

pureRSSI_t perform(bool break_on_operation, int i, freq_t f, int tracking)     // Measure the RSSI for one frequency, used from sweep and other measurement routines. Must do all HW setup
{
  int modulation_delay = 0;
  int modulation_index = 0;
  int spur_second_pass = false;
  if (i == 0 && dirty ) {                                                        // if first point in scan and dirty
    calculate_correction();                                                 // pre-calculate correction factor dividers to avoid float division
    apply_settings();                                                       // Initialize HW
    scandirty = true;                                                       // This is the first pass with new settings
    dirty = false;
    sweep_elapsed = chVTGetSystemTimeX();                              // for measuring accumulated time
    // Set for actual time pre calculated value (update after sweep)
    setting.actual_sweep_time_us = calc_min_sweep_time_us();
    // Change actual sweep time as user input if it greater minimum
    // And set start delays for 1 run
    // manually set delay, for better sync
    if (setting.sweep_time_us < 2.5 * ONE_MS_TIME){
      setting.additional_step_delay_us = 0;
      setting.sweep_time_us = 0; // set minimum
    }
    else if (setting.sweep_time_us <= 3 * ONE_MS_TIME){
      setting.additional_step_delay_us = 1;
      setting.sweep_time_us = 3000;
    }
    else if (setting.sweep_time_us > setting.actual_sweep_time_us){
      setting.additional_step_delay_us = (setting.sweep_time_us - setting.actual_sweep_time_us)/(sweep_points);
      setting.actual_sweep_time_us = setting.sweep_time_us;
    }
    else{ // not add additional correction, apply recommend time
      setting.additional_step_delay_us = 0;
      //      setting.sweep_time_us = setting.actual_sweep_time_us;
    }
    if (MODE_INPUT(setting.mode)) {
      calculate_static_correction();
      if (!in_selftest) clock_above_48MHz();
      is_below = false;
    }
    //    if (MODE_OUTPUT(setting.mode) && setting.additional_step_delay_us < 500)     // Minimum wait time to prevent LO from lockup during output frequency sweep
    //      setting.additional_step_delay_us = 500;
    // Update grid and status after
    if (break_on_operation  && MODE_INPUT(setting.mode)) {                       // during normal operation
      redraw_request |= REDRAW_CAL_STATUS;
      if (FREQ_IS_CW()) {                                       // if zero span mode
        update_grid();                                          // and update grid and frequency
      }
    }
  }

  if (setting.mode == M_GENLOW && ( setting.frequency_step != 0 || setting.level_sweep != 0.0)) {// start burst
    SI4432_Sel = SI4432_LO ;
    if (i == 0) {
//      set_switch_transmit();
      SI4432_Write_Byte(SI4432_GPIO2_CONF, 0x1D) ; // Set GPIO2 output to ground
    } else if (i == 1) {
//      set_switch_off();
      SI4432_Write_Byte(SI4432_GPIO2_CONF, 0x1F) ; // Set GPIO2 output to ground
    }
  }

  if (setting.mode == M_GENLOW && ( setting.frequency_step != 0 || setting.level_sweep != 0.0 || i == 0)) {// if in low output mode and level sweep or frequency weep is active or at start of sweep
    float ls=setting.level_sweep;                                           // calculate and set the output level
    if (ls > 0)
      ls += 0.5;
    else
      ls -= 0.5;
    float a = ((int)((setting.level + ((float)i / sweep_points) * ls)*2.0)) / 2.0;
    a += PURE_TO_float(get_frequency_correction(f));
    if (a != old_a) {
      old_a = a;
#ifdef __SI4432__
      int d = 0;              // Start at lowest drive level;
      a = a + POWER_OFFSET;
      if (a > 0) {
        d++;
        a = a - 3;
      }
      if (a > 0) {
        d++;
        a = a - 3;
      }
      if (a > 0) {
        d++;
        a = a - 3;
      }
      SI4432_Sel = SI4432_RX ;
      SI4432_Drive(d);
      if (a > 0)
        a = 0;

      if( a >  - SWITCH_ATTENUATION) {
        set_switch_transmit();
      } else {
        a = a + SWITCH_ATTENUATION;
        set_switch_receive();
      }
#endif
      if (a < -31)
        a = -31;
      a = -a;
#ifdef __PE4302__
      PE4302_Write_Byte((int)(a * 2) );
#endif
    }
  }
  if (setting.mode == M_LOW && S_IS_AUTO(setting.agc) && !check_for_AM && UNIT_IS_LOG(setting.unit)) {   // If in low input mode with auto AGC and log unit
#ifdef __SI4432__
    if (f < 1500000)
      auto_set_AGC_LNA(false, f*9/1500000);
    else
      auto_set_AGC_LNA(true, 0);
#endif
  }
  // Calculate the RSSI correction for later use
  if (MODE_INPUT(setting.mode)){ // only cases where the value can change on 0 point of sweep
    if (i == 0 || setting.frequency_step != 0)
      correct_RSSI_freq = get_frequency_correction(f);
  }
  int *current_fm_modulation = 0;
  if (MODE_OUTPUT(setting.mode)) {
    if (setting.modulation != MO_NONE && setting.modulation != MO_EXTERNAL && setting.modulation_frequency != 0) {
      modulation_delay = (1000000/ MODULATION_STEPS ) / setting.modulation_frequency;     // 5 steps so 1MHz/5
      modulation_counter = 0;
      if (setting.modulation == MO_AM)          // -14 default
        modulation_delay += config.cor_am;
      else  {                               // must be FM
        if (setting.modulation == MO_WFM) {         // -17 default
          modulation_delay += config.cor_wfm;
          modulation_index = 1;
        } else {                                // must be NFM
          modulation_delay += config.cor_nfm;  // -17 default
          // modulation_index = 0; // default value
        }
        if ((setting.mode == M_GENLOW  && f > ((freq_t)480000000) - DEFAULT_IF) ||
            (setting.mode == M_GENHIGH  && f > ((freq_t)480000000) ) )
          modulation_index += 2;
        current_fm_modulation = (int *)fm_modulation[modulation_index];
        f -= fm_modulation_offset[modulation_index];           // Shift output frequency
      }
    }
  }
modulation_again:
  // -----------------------------------------------------  modulation for output modes ---------------------------------------
  if (MODE_OUTPUT(setting.mode)){
    if (setting.modulation == MO_AM) {               // AM modulation
      int p = setting.attenuate_x2 + am_modulation[modulation_counter];
      if      (p>63) p = 63;
      else if (p< 0) p =  0;
#ifdef __PE4302__
      PE4302_Write_Byte(p);
#endif
    }
    else if (current_fm_modulation) { // setting.modulation == MO_NFM || setting.modulation == MO_WFM  //FM modulation
#ifdef __SI4432__
      SI4432_Sel = SI4432_LO ;
      int offset = current_fm_modulation[modulation_counter];
      SI4432_Write_2_Byte(SI4432_FREQ_OFFSET1, (offset & 0xff ), ((offset >> 8) & 0x03 ));  // Use frequency hopping channel for FM modulation
//      SI4432_Write_Byte(SI4432_FREQ_OFFSET2, );  // Use frequency hopping channel for FM modulation
#endif
    }
    modulation_counter++;
    if (modulation_counter == MODULATION_STEPS)  // 3dB modulation depth
      modulation_counter = 0;
    if (setting.modulation != MO_NONE && setting.modulation != MO_EXTERNAL) {
      my_microsecond_delay(modulation_delay);
    }
  }

  // -------------------------------- Acquisition loop for one requested frequency covering spur avoidance and vbwsteps ------------------------
  pureRSSI_t RSSI = float_TO_PURE_RSSI(-150);
//#define __DEBUG_SPUR__
#ifdef __DEBUG_SPUR__                 // For debugging the spur avoidance control
  stored_t[i] = -90.0;                                  // Display when to do spur shift in the stored trace
#endif
  int t = 0;
  do {
    freq_t lf = f;
    if (vbwSteps > 1) {          // Calculate sub steps
      int offs_div10 = (t - (vbwSteps >> 1)) * 500 / 10; // steps of half the rbw
      if ((vbwSteps & 1) == 0)                           // Uneven steps, center
        offs_div10+= 250 / 10;                           // Even, shift half step
      int offs = offs_div10 * actual_rbw_x10;
      if (setting.step_delay_mode == SD_PRECISE)
        offs>>=1;                                        // steps of a quarter rbw
      lf += offs;
    }

// -------------- Calculate the IF -----------------------------

    if (/* MODE_INPUT(setting.mode) && */ i > 0 && FREQ_IS_CW())              // In input mode in zero span mode after first setting of the LO's
      goto skip_LO_setting;                                             // No more LO changes required, save some time and jump over the code

    freq_t local_IF;
    spur_second_pass = false;
  again:                                                              // Spur reduction jumps to here for second measurement

    local_IF=0;                                                         // to get rid of warning
    if (MODE_HIGH(setting.mode)) {
      local_IF = 0;
    } else if (MODE_LOW(setting.mode)){                                              // All low mode
      if (!setting.auto_IF)
        local_IF = setting.frequency_IF;
      else
        local_IF = DEFAULT_IF;
      if (setting.mode == M_LOW) {
        if (tracking) {                                // VERY SPECIAL CASE!!!!!   Measure BPF
#if 0                                                               // Isolation test
          local_IF = lf;
          lf = 0;
#else
          local_IF += lf - reffer_freq[setting.refer];    // Offset so fundamental of reffer is visible
          lf = reffer_freq[setting.refer];
#endif
        } else {
          if(!in_selftest && avoid_spur(lf)) {         // check if alternate IF is needed to avoid spur.
            local_IF = spur_alternate_IF;
#ifdef __DEBUG_SPUR__                 // For debugging the spur avoidance control
            stored_t[i] = -60.0;                                       // Display when to do spur shift in the stored trace
#endif
          }
          if (S_STATE(setting.spur_removal)){         // If in low input mode and spur reduction is on
            if (S_IS_AUTO(setting.below_IF) && (lf < local_IF / 2  || lf > local_IF) ) // if below 150MHz and auto_below_IF  <-------------------TODO ---------------------
            {              // else low/above IF
              if (spur_second_pass)
                setting.below_IF = S_AUTO_ON;               // use below IF in second pass
              else
                setting.below_IF = S_AUTO_OFF;              // and above IF in first pass
            }
            else
            {
              if (spur_second_pass)                       // If second spur pass
#ifdef __SI4432__
                local_IF  = local_IF + 500000;           // apply IF spur shift
#else
              local_IF  = local_IF + 1000000;           // apply IF spur shift
#endif
            }
          }
        }
      } else {              // Output mode
        if (setting.modulation == MO_EXTERNAL)    // VERY SPECIAL CASE !!!!!! LO input via high port
          local_IF += lf;
      }
#ifdef __ULTRA__
    } else if (setting.mode == M_ULTRA) {               // No above/below IF mode in Ultra
      local_IF  = setting.frequency_IF + (int)(actual_rbw < 350.0 ? S_STATE(setting.spur_removal)*300000 : 0 );
#ifdef __SI4432__
      set_freq (SI4432_RX , local_IF);
#endif
      //     local_IF  = setting.frequency_IF + (int)(actual_rbw < 300.0?S_STATE(setting.spur_removal) * 1000 * actual_rbw:0);
#endif
    }

    // ------------- Set LO ---------------------------

#ifdef __ULTRA__
    if (setting.mode == M_ULTRA) {      // Set LO to correct harmonic in Ultra mode
      //      if (lf > 3406000000 )
      //        setFreq (1, local_IF/5 + lf/5);
      //      else
      if (S_STATE(setting.spur_removal) != 1) {  // Left of tables
        if (lf > 3250000000 )
          set_freq (SI4432_LO , lf/5 - local_IF/5);
        if (lf > 1250000000 )
          set_freq (SI4432_LO, lf/3 - local_IF/3);
        else
          set_freq (SI4432_LO,  lf - local_IF);

      } else {              // Right of tables
        if (lf >= 2350000000)
          set_freq (SI4432_LO,  lf/5 + local_IF/5);
        else
          set_freq (SI4432_LO, lf/3 + local_IF/3);
      }
    } else
#endif
    {                                           // Else set LO ('s)
      freq_t target_f;
      if (setting.mode == M_LOW && !setting.tracking && S_STATE(setting.below_IF)) // if in low input mode and below IF
        target_f = local_IF-lf;                                                 // set LO SI4432 to below IF frequency
      else
        target_f = local_IF+lf;                                                 // otherwise to above IF
#ifdef __SI4432__
      set_freq (SI4432_LO, target_f);                                                 // otherwise to above IF
#endif
    }

#if 1               // No 72MHz spur avoidance yet
        if (setting.mode == M_LOW && !in_selftest /* && !(SDU1.config->usbp->state == USB_ACTIVE) */ ) {         // Avoid 72MHz spur
          int set_below = false;
#ifdef TINYSA4
          if (lf < 40000000) {
            uint32_t tf = lf;
            while (tf > 4000000) tf -= 4000000;
            if (tf < 2000000 )
              set_below = true;
          } else
#endif
          if (lf > 40000000){
            uint32_t tf = lf;
            while (tf > 48000000) tf -= 48000000;
            if (tf < 20000000 )
              set_below = true;
          }
          if (set_below) {     // If below 48MHz
            if (!is_below) {
              clock_below_48MHz();
              is_below = true;
            }
          } else {
            if (is_below) {
              clock_above_48MHz();
              is_below = false;
            }
          }
        }
#endif

// ----------- Set IF ------------------

    if (local_IF != 0)
    {
#ifdef __SI4432__
      set_freq (SI4432_RX , local_IF);
#endif
    }

    if (MODE_OUTPUT(setting.mode)) {
#ifdef __SI4432__
      my_microsecond_delay(200);                 // To prevent lockup of SI4432
#endif
    }


    // ------------------------- end of processing when in output mode ------------------------------------------------

    skip_LO_setting:
    if (i == 0 && t == 0)                                                   // if first point in scan (here is get 1 point data)
      start_of_sweep_timestamp = chVTGetSystemTimeX();                      // initialize start sweep time

    if (MODE_OUTPUT(setting.mode)) {               // No substepping and no RSSI in output mode
      if (break_on_operation && operation_requested)          // break subscanning if requested
        return(0);         // abort
      if ( i==1 && MODE_OUTPUT(setting.mode) && setting.modulation != MO_NONE && setting.modulation != MO_EXTERNAL) { // if in output mode with modulation and LO setup done
//        i = 1;              // Everything set so skip LO setting
        goto modulation_again;                                             // Keep repeating sweep loop till user aborts by input
      }
      return(0);
    }
    // ---------------- Prepare RSSI ----------------------

    // jump here if in zero span mode and all HW frequency setup is done.

#ifdef __FAST_SWEEP__
#ifdef __SI4432__
    if (i == 0 && setting.frequency_step == 0 && setting.trigger == T_AUTO && S_STATE(setting.spur_removal) == 0 && SI4432_step_delay == 0 && setting.repeat == 1 && setting.sweep_time_us < 100*ONE_MS_TIME) {
      // if ultra fast scanning is needed prefill the SI4432 RSSI read buffer
      SI4432_Fill(MODE_SELECT(setting.mode), 0);
    }
#endif
#endif
    pureRSSI_t pureRSSI;
    //    if ( i < 3)
    //      shell_printf("%d %.3f %.3f %.1f\r\n", i, local_IF/1000000.0, lf/1000000.0, subRSSI);

    // ************** trigger mode if need
    // trigger on measure 4 point
#define T_POINTS            4
#define T_LEVEL_UNDEF       (1<<(16-T_POINTS)) // should drop after 4 shifts left
#define T_LEVEL_BELOW       1
#define T_LEVEL_ABOVE       0
    // Trigger mask, should have width T_POINTS bit
#define T_DOWN_MASK         (0b0011)           // 2 from up 2 to bottom
#define T_UP_MASK           (0b1100)           // 2 from bottom 2 to up
#define T_LEVEL_CLEAN       ~(1<<T_POINTS)     // cleanup old trigger data

    if (i == 0 && setting.frequency_step == 0 && setting.trigger != T_AUTO) { // if in zero span mode and wait for trigger to happen and NOT in trigger mode

#if 1
      volatile uint8_t trigger_lvl = PURE_TO_DEVICE_RSSI((int16_t)((float_TO_PURE_RSSI(setting.trigger_level) - correct_RSSI - correct_RSSI_freq)));
      SI4432_trigger_fill(MODE_SELECT(setting.mode), trigger_lvl, (setting.trigger_direction == T_UP), setting.trigger_mode);
#else
      register uint16_t t_mode;
      pureRSSI_t trigger_lvl;
      uint16_t data_level = T_LEVEL_UNDEF;
      // Calculate trigger level
      trigger_lvl = float_TO_PURE_RSSI(setting.trigger_level) - correct_RSSI - correct_RSSI_freq;

      if (setting.trigger_direction == T_UP)
        t_mode = T_UP_MASK;
      else
        t_mode = T_DOWN_MASK;
      uint32_t additional_delay = 0;// reduce noise
      if (setting.sweep_time_us >= 100*ONE_MS_TIME) additional_delay = 20;
#ifdef __SI4432__
      SI4432_Sel =  MODE_SELECT(setting.mode);
#endif
      do{                                                 // wait for trigger to happen
#ifdef __SI4432__
        pureRSSI = DEVICE_TO_PURE_RSSI((deviceRSSI_t)SI4432_Read_Byte(SI4432_REG_RSSI));
#endif
        if (break_on_operation && operation_requested)                        // allow aborting a wait for trigger
          return 0;                                                           // abort

        // Store data level bitfield (remember only last 2 states)
        // T_LEVEL_UNDEF mode bit drop after 2 shifts
        data_level = ((data_level<<1) | (pureRSSI < trigger_lvl ? T_LEVEL_BELOW : T_LEVEL_ABOVE))&(T_LEVEL_CLEAN);
        if (data_level == t_mode)  // wait trigger
          break;
        if (additional_delay)
          my_microsecond_delay(additional_delay);
      }while(1);
#ifdef __FAST_SWEEP__
#ifdef __SI4432__
      if (S_STATE(setting.spur_removal) == 0 && SI4432_step_delay == 0 && setting.repeat == 1 && setting.sweep_time_us < 100*ONE_MS_TIME) {
        SI4432_Fill(MODE_SELECT(setting.mode), 1);                       // fast mode possible to pre-fill RSSI buffer
      }
#endif
#endif

#endif
      if (setting.trigger == T_SINGLE) {
        set_trigger(T_DONE);
      }
      start_of_sweep_timestamp = chVTGetSystemTimeX();
    }
    //else
    {
#ifdef __SI4432__
      pureRSSI = SI4432_RSSI(lf, MODE_SELECT(setting.mode));            // Get RSSI, either from pre-filled buffer
#endif
    }
#ifdef __SPUR__
    static pureRSSI_t spur_RSSI = -1;                               // Initialization only to avoid warning.
    if (setting.mode == M_LOW && S_STATE(setting.spur_removal)) {
      if (!spur_second_pass) {                                        // If first spur pass
        spur_RSSI = pureRSSI;                                       // remember measure RSSI
        spur_second_pass = true;
        goto again;                                                 // Skip all other processing
      } else {                              // If second  spur pass
        pureRSSI = ( pureRSSI < spur_RSSI ? pureRSSI : spur_RSSI);  // Take minimum of two
        if (S_IS_AUTO(setting.below_IF))
          setting.below_IF = S_AUTO_OFF;                            // make sure it is off for next pass
      }
    }
#endif

    if (RSSI < pureRSSI)                                     // Take max during subscanning
      RSSI = pureRSSI;
    t++;                                                    // one subscan done
    if (break_on_operation && operation_requested)          // break subscanning if requested
      break;         // abort
  } while (t < vbwSteps);                                   // till all sub steps done
  return RSSI + correct_RSSI + correct_RSSI_freq; // add correction
}

#define MAX_MAX 4
int16_t max_index[MAX_MAX];
int16_t cur_max = 0;

static int low_count = 0;
static int sweep_counter = 0;           // Only used for HW refresh

// main loop for measurement
static bool sweep(bool break_on_operation)
{
  float RSSI;
  int16_t downslope;
#ifdef __SI4432__
  uint32_t agc_peak_freq = 0;
  float agc_peak_rssi = -150;
  float agc_prev_rssi = -150;
  int last_AGC_value = 0;
  uint8_t last_AGC_direction_up = false;
  int AGC_flip_count = 0;
#endif
  //  if (setting.mode== -1)
  //    return;
  //  START_PROFILE;
#ifdef TINYSA3
  palClearPad(GPIOB, GPIOB_LED);
#endif
#ifdef TINYSA4
  palClearPad(GPIOC, GPIOC_LED);
#endif
  downslope = true;             // Initialize the peak search algorithm
  temppeakLevel = -150;
  float temp_min_level = 100;

  //  spur_old_stepdelay = 0;
  //  shell_printf("\r\n");

  modulation_counter = 0;                                             // init modulation counter in case needed
  int refreshing = false;

  if (dirty) {                    // Calculate new scanning solution
    sweep_counter = 0;
    if (get_sweep_frequency(ST_SPAN) < 300000)  // Check if AM signal
      check_for_AM = true;
    else {
      signal_is_AM = false;
      check_for_AM = false;
    }
  } else if ( MODE_INPUT(setting.mode) && setting.frequency_step > 0) {
    sweep_counter++;
    if (sweep_counter > 50 ) {     // refresh HW after 50 sweeps
      dirty = true;
      refreshing = true;
      sweep_counter = 0;
    }
  }

again:                          // Waiting for a trigger jumps back to here
  setting.measure_sweep_time_us = 0;                   // start measure sweep time
//  start_of_sweep_timestamp = chVTGetSystemTimeX();    // Will be set in perform

sweep_again:                                // stay in sweep loop when output mode and modulation on.

  // ------------------------- start sweep loop -----------------------------------
  for (int i = 0; i < sweep_points; i++) {
    // --------------------- measure -------------------------

    RSSI = PURE_TO_float(perform(break_on_operation, i, frequencies[i], setting.tracking));    // Measure RSSI for one of the frequencies
    // if break back to top level to handle ui operation
    if (refreshing)
      scandirty = false;
    if (break_on_operation && operation_requested) {                        // break loop if needed
      if (setting.actual_sweep_time_us > ONE_SECOND_TIME && MODE_INPUT(setting.mode)) {
        ili9341_set_background(LCD_BG_COLOR);
        ili9341_fill(OFFSETX, CHART_BOTTOM+1, WIDTH, 1);                    // Erase progress bar
      }
      return false;
    }

    dacPutChannelX(&DACD2, 0, (((float)i)*config.sweep_voltage)*4.279);        // Output sweep voltage  4095 -> 3.3 Volt

    // ----------------------- in loop AGC ---------------------------------

#ifdef __SI4432__
    if (!in_selftest && setting.mode == M_HIGH && S_IS_AUTO(setting.agc) && UNIT_IS_LOG(setting.unit)) {
#define AGC_RSSI_THRESHOLD  (-55+get_attenuation())

      if (RSSI > AGC_RSSI_THRESHOLD && RSSI > agc_prev_rssi) {
        agc_peak_freq = frequencies[i];
        agc_peak_rssi = agc_prev_rssi = RSSI;
      }
      if (RSSI < AGC_RSSI_THRESHOLD)
        agc_prev_rssi = -150;
      freq_t delta_freq = frequencies[i] - agc_peak_freq;
      if (agc_peak_freq != 0 &&  delta_freq < 2000000) {
        int max_gain = (-25 - agc_peak_rssi ) / 4;
        auto_set_AGC_LNA(false, 16 + delta_freq * max_gain / 2000000 );    // enable LNA   and stepwise gain
      }
      else
        auto_set_AGC_LNA(TRUE, 0);
    }
#endif

    // Delay between points if needed, (all delays can apply in SI4432 fill)
    if (setting.measure_sweep_time_us == 0){                                    // If not already in buffer
      if (setting.additional_step_delay_us && (MODE_INPUT(setting.mode) || setting.modulation == MO_NONE)) {     // No delay when modulation is active
        if (setting.additional_step_delay_us < 30*ONE_MS_TIME)                                                   // Maximum delay time using my_microsecond_delay
          my_microsecond_delay(setting.additional_step_delay_us);
        else
          osalThreadSleepMilliseconds(setting.additional_step_delay_us / ONE_MS_TIME);
      }
    }

    if (MODE_INPUT(setting.mode)) {

      if (setting.actual_sweep_time_us > ONE_SECOND_TIME && (i & 0x07) == 0) {  // if required
    	int pos = i * (WIDTH+1) / sweep_points;
    	ili9341_set_background(LCD_SWEEP_LINE_COLOR);
        ili9341_fill(OFFSETX, CHART_BOTTOM+1, pos, 1);     // update sweep progress bar
        ili9341_set_background(LCD_BG_COLOR);
        ili9341_fill(OFFSETX+pos, CHART_BOTTOM+1, WIDTH-pos, 1);
      }

      // ------------------------ do all RSSI calculations from CALC menu -------------------

      if (setting.average != AV_OFF)
        temp_t[i] = RSSI;
      if (setting.subtract_stored) {
        RSSI = RSSI - stored_t[i] + setting.normalize_level;
      }
#ifdef __SI4432__
//#define __DEBUG_AGC__
#ifdef __DEBUG_AGC__                 // For debugging the AGC control
      stored_t[i] = (SI4432_Read_Byte(0x69) & 0x01f) * 3.0 - 90.0; // Display the AGC value in the stored trace
#endif

      if (check_for_AM) {
        int AGC_value = (SI4432_Read_Byte(0x69) & 0x01f) * 3 - 90;
        if (AGC_value < last_AGC_value &&  last_AGC_direction_up ) {
          AGC_flip_count++;
        } else if (AGC_value > last_AGC_value &&  !last_AGC_direction_up ) {
          AGC_flip_count++;
        }
        last_AGC_value = AGC_value;
      }
#endif
      if (scandirty || setting.average == AV_OFF) {             // Level calculations
        if (setting.average == AV_MAX_DECAY) age[i] = 0;
        actual_t[i] = RSSI;
      } else {
        switch(setting.average) {
        case AV_MIN:      if (actual_t[i] > RSSI) actual_t[i] = RSSI; break;
        case AV_MAX_HOLD: if (actual_t[i] < RSSI) actual_t[i] = RSSI; break;
        case AV_MAX_DECAY:
          if (actual_t[i] < RSSI) {
            age[i] = 0;
            actual_t[i] = RSSI;
          } else {
            if (age[i] > setting.decay)
              actual_t[i] -= 0.5;
            else
              age[i] += 1;
          }
          break;
        case AV_4:  actual_t[i] = (actual_t[i]*3 + RSSI) / 4.0; break;
        case AV_16: actual_t[i] = (actual_t[i]*15 + RSSI) / 16.0; break;
#ifdef __QUASI_PEAK__
        case AV_QUASI:
          { static float old_RSSI = -150.0;
          if (i == 0) old_RSSI = actual_t[sweep_points-1];
          if (RSSI > old_RSSI && setting.attack > 1)
             old_RSSI += (RSSI - old_RSSI)/setting.attack;
          else if (RSSI < old_RSSI && setting.decay > 1)
            old_RSSI += (RSSI - old_RSSI)/setting.decay;
          else
            old_RSSI = RSSI;
          actual_t[i] = old_RSSI;
          }
          break;
#endif
        }
      }

      if (temp_min_level > actual_t[i])   // Remember minimum
        temp_min_level = actual_t[i];

      // --------------------------- find peak and add to peak table if found  ------------------------


      // START_PROFILE
      if (i == 0) {                                          // Prepare peak finding
        cur_max = 0;          // Always at least one maximum
        temppeakIndex = 0;
        temppeakLevel = actual_t[i];
        max_index[0] = 0;
        downslope = true;
      }
      if (downslope) {                               // If in down slope peak finding
        if (temppeakLevel > actual_t[i]) {           // Follow down
          temppeakIndex = i;                         // Latest minimum
          temppeakLevel = actual_t[i];
        } else if (temppeakLevel + setting.noise < actual_t[i] ) {    // Local minimum found
          temppeakIndex = i;                         // This is now the latest maximum
          temppeakLevel = actual_t[i];
          downslope = false;
        }
      } else {                                      // up slope peak finding
        if (temppeakLevel < actual_t[i]) {    // Follow up
          temppeakIndex = i;
          temppeakLevel = actual_t[i];
        } else if (actual_t[i] < temppeakLevel - setting.noise) {    // Local max found

          // maintain sorted peak table
          int j = 0;                                            // Insert max in sorted table
          while (j<cur_max && actual_t[max_index[j]] >= temppeakLevel)   // Find where to insert
            j++;
          if (j < MAX_MAX) {                                    // Larger then one of the previous found
            int k = MAX_MAX-1;
            while (k > j) {                                      // Shift to make room for max
              max_index[k] = max_index[k-1];
              //              maxlevel_index[k] = maxlevel_index[k-1];        // Only for debugging
              k--;
            }
            max_index[j] = temppeakIndex;
            //            maxlevel_index[j] = actual_t[temppeakIndex];      // Only for debugging
            if (cur_max < MAX_MAX) {
              cur_max++;
            }
            //STOP_PROFILE
          }
          // Insert done
          temppeakIndex = i;            // Latest minimum
          temppeakLevel = actual_t[i];

          downslope = true;
        }
      }        // end of peak finding
    }           // end of input specific processing
  }  // ---------------------- end of sweep loop -----------------------------

  if (MODE_OUTPUT(setting.mode) && setting.modulation != MO_NONE ) // if in output mode with modulation
    goto sweep_again;                                             // Keep repeating sweep loop till user aborts by input

  // --------------- check if maximum is above trigger level -----------------

  if (setting.trigger != T_AUTO && setting.frequency_step > 0) {    // Trigger active
    if (actual_t[max_index[0]] < setting.trigger_level) {
      goto again;                                                   // not yet, sweep again
    } else {
      if (setting.trigger == T_SINGLE) {
        set_trigger(T_DONE);
      }
    }
//    scandirty = true;                // To show trigger happened
  }
  if (setting.actual_sweep_time_us > ONE_SECOND_TIME && MODE_INPUT(setting.mode)) {
    // ili9341_fill(OFFSETX, CHART_BOTTOM+1, WIDTH, 1, 0);     // Erase progress bar before updating actual_sweep_time
    ili9341_set_background(LCD_BG_COLOR);
    ili9341_fill(OFFSETX, CHART_BOTTOM+1, WIDTH, 1);
  }
  // ---------------------- process measured actual sweep time -----------------
  // For CW mode value calculated in SI4432_Fill
  if (setting.measure_sweep_time_us == 0)
    setting.measure_sweep_time_us = (chVTGetSystemTimeX() - start_of_sweep_timestamp) * 100;

  // Update actual time on change on status panel
  uint32_t delta = abs((int)(setting.actual_sweep_time_us - setting.measure_sweep_time_us));
  if ((delta<<3) > setting.actual_sweep_time_us){ // update if delta > 1/8
    redraw_request|=REDRAW_CAL_STATUS;
  }
  setting.actual_sweep_time_us = setting.measure_sweep_time_us;
  // Not possible reduce sweep time, it minimum!
  if (setting.sweep_time_us < setting.actual_sweep_time_us && setting.additional_step_delay_us == 0){
// Warning!! not correct set sweep time here, you get error!!
// value update to real and after + recalculated additional delay
//    setting.sweep_time_us = setting.actual_sweep_time_us;
//    redraw_request |= REDRAW_CAL_STATUS;
//    if (FREQ_IS_CW())                                  // if zero span mode
//      update_grid();
  }
  else{
    uint32_t dt = 0;
    static uint32_t last_dt = 0;
    // selected time less then actual, need reduce delay
    if (setting.sweep_time_us < setting.actual_sweep_time_us){
      dt = (setting.actual_sweep_time_us - setting.sweep_time_us)/(sweep_points);
      if (setting.additional_step_delay_us > dt)
        setting.additional_step_delay_us-=dt;
      else
        setting.additional_step_delay_us = 0;
    }// selected time greater then actual, need increase delay
    else if (setting.sweep_time_us > setting.actual_sweep_time_us){
      dt = (setting.sweep_time_us - setting.actual_sweep_time_us)/(sweep_points);
      setting.additional_step_delay_us+=dt;
    }
    // Update info on correction on next step, after apply . Always show when changed
    if (last_dt /* && dt == 0 */){
      redraw_request|=REDRAW_CAL_STATUS;
      if (FREQ_IS_CW())                // if zero span mode
        update_grid();                 // and update grid and frequency
    }
    last_dt = dt;
  }

  // ---------------------- sweep finished,  do all postprocessing ---------------------

  if (scandirty) {
    scandirty = false;
    redraw_request |= REDRAW_CAL_STATUS;
  }

  if (MODE_OUTPUT(setting.mode) )                            // Sweep time is calculated, we can sweep again in output mode
    goto again;                                             // Keep repeating sweep loop till user aborts by input

#define __MIRROR_MASKING__
#ifdef __MIRROR_MASKING__
#ifdef __SI4432__
  if (setting.mode == M_HIGH && setting.mirror_masking) {
    int mirror_offset = 2 * 937000 / setting.frequency_step;
//    int mask_start = 0;
//    int mask_end = 0;
    if (mirror_offset > 3) {
      for (int i = 1; i < sweep_points - mirror_offset; i++) {
        int m = i+mirror_offset;
        if (actual_t[i] > -80 && actual_t[m] < actual_t[i] - 25 && ( actual_t[m] > actual_t[m-1] || actual_t[m+1] > actual_t[m-1] ) /* && (i < mask_start || mask_start == 0) */ ) {
//          if (mask_start == 0)
//            mask_start = m;
          actual_t[m] = actual_t[m-1];
          actual_t[m+1] = actual_t[m-1];
        }
//        else {
//          if (i == mask_start)
//            i += mirror_offset;
//          mask_start =0;
//        }
      }
    }
  }

#endif
#endif

  // -------------------------- auto attenuate ----------------------------------
#define AUTO_TARGET_LEVEL   -25
#define AUTO_TARGET_WINDOW  2

  if (!in_selftest && setting.mode == M_LOW && setting.auto_attenuation) {  // calculate and apply auto attenuate
    setting.atten_step = false;     // No step attenuate in low mode auto attenuate
    int changed = false;
    int delta = 0;
    int actual_max_level = (max_index[0] == 0 ? -100 :(int) (actual_t[max_index[0]] - get_attenuation()) ); // If no max found reduce attenuation
    if (actual_max_level < AUTO_TARGET_LEVEL && setting.attenuate_x2 > 0) {
      delta = - (AUTO_TARGET_LEVEL - actual_max_level);
    } else if (actual_max_level > AUTO_TARGET_LEVEL && setting.attenuate_x2 < 60) {
      delta = actual_max_level - AUTO_TARGET_LEVEL;
    }
    if ((chVTGetSystemTimeX() - sweep_elapsed > 10000 && delta != 0) || delta > 5 ) {
      setting.attenuate_x2 += delta + delta;
      if (setting.attenuate_x2 < 0)
        setting.attenuate_x2= 0;
      if (setting.attenuate_x2 > 60)
        setting.attenuate_x2 = 60;
      changed = true;
      sweep_elapsed = chVTGetSystemTimeX();
    }

    // Try update settings
    if (changed){
#ifdef __PE4302__
      PE4302_Write_Byte((int) get_attenuation() * 2);
#endif
      redraw_request |= REDRAW_CAL_STATUS;
#ifdef __SI4432__
      SI4432_Sel = SI4432_RX ;
#if 0                               // this should never happen
      if (setting.atten_step) {
        set_switch_transmit();          // This should never happen
      } else {
        set_switch_receive();
      }
#endif
#endif
      calculate_static_correction();            // Update correction
//      dirty = true;                               // Needed to recalculate the correction factor
    }
  }

  // ----------------------------------  auto AGC ----------------------------------


  if (!in_selftest && MODE_INPUT(setting.mode)) {
#ifdef __SI4432__
    if (S_IS_AUTO(setting.agc)) {
      int actual_max_level = actual_t[max_index[0]] - get_attenuation();        // No need to use float
      if (UNIT_IS_LINEAR(setting.unit)) { // Auto AGC in linear mode
        if (actual_max_level > - 45)
          auto_set_AGC_LNA(false, 0); // Strong signal, no AGC and no LNA
        else
          auto_set_AGC_LNA(TRUE, 0);
      }
      if (check_for_AM) {
        if (signal_is_AM) {
          if (actual_max_level < - 40 )
            signal_is_AM = false;
        } else {
          if (AGC_flip_count > 20 && actual_max_level >= - 40)
            signal_is_AM = true;
        }
        if (signal_is_AM) {      // if log mode and AM signal
          auto_set_AGC_LNA(false, 16); // LNA on and no AGC
        } else {
          auto_set_AGC_LNA(TRUE, 0);
        }
      }
    } else
#endif
      signal_is_AM = false;
  }


  // -------------------------- auto reflevel ---------------------------------
  if (max_index[0] > 0)
    temppeakLevel = actual_t[max_index[0]];

  if (!in_selftest && MODE_INPUT(setting.mode) && setting.auto_reflevel) {  // Auto reflevel

    float r = value(temppeakLevel);
    float s_max = r / setting.scale;                  // Peak level normalized to /div

    if (UNIT_IS_LINEAR(setting.unit)) {            // Linear scales can not have negative values
      if (setting.reflevel > REFLEVEL_MIN)  {
        if (s_max <  2)
          low_count = 5;
        else if (s_max < 4)
          low_count++;
        else
          low_count = 0;
      }
      if ((low_count > 4) || (setting.reflevel < REFLEVEL_MAX && s_max > NGRIDY) ) { // ensure minimum and maximum reflevel
        if (r < REFLEVEL_MIN)
          r = REFLEVEL_MIN;
        if (r > REFLEVEL_MAX)
          r = REFLEVEL_MAX;
        if (r != setting.reflevel) {
          //if (setting.scale * NGRIDY > r)
          set_scale(r / NGRIDY);
          set_reflevel(setting.scale*NGRIDY);
          //         dirty = false;                        // Prevent reset of SI4432
        }
      }
    } else {
#define MAX_FIT (NGRIDY-1.2)
      float s_min = value(temp_min_level)/setting.scale;
      float s_ref = setting.reflevel/setting.scale;
      if (s_max < s_ref  - NGRIDY || s_min > s_ref) { //Completely outside
        if (s_max - s_min < NGRIDY - 2)
          set_reflevel(setting.scale*(floor(s_min+8.8+ 1)));
        else
          set_reflevel(setting.scale*(floor(s_max)+1));
        //        dirty = true;                               // Must be  above if(scandirty!!!!!)
      }else if (s_max > s_ref  - 0.5 || s_min > s_ref - 8.8 ) { // maximum to high or minimum to high
        set_reflevel(setting.reflevel + setting.scale);
        //        dirty = true;                               // Must be  above if(scandirty!!!!!)
      } else if (s_min < s_ref - 10.1 && s_max < s_ref -  1.5) { // minimum too low and maximum can move up
        set_reflevel(setting.reflevel - setting.scale);
        //        dirty = true;                               // Must be  above if(scandirty!!!!!)
      }
      //     dirty = false;                        // Prevent reset of SI4432
    }
  }

  // --------------------- set tracking markers from maximum table -----------------

  if (MODE_INPUT(setting.mode)) {               // Assign maxima found to tracking markers
    int i = 0;
    int m = 0;
    while (i < cur_max) {                                 // For all maxima found
      while (m < MARKERS_MAX) {
        if (markers[m].enabled && markers[m].mtype & M_TRACKING) {   // Available marker found
          markers[m].index = max_index[i];
          interpolate_maximum(m);
          // markers[m].frequency = frequencies[markers[m].index];
#if 0
          float v = actual_t[markers[m].index] - 10.0;              // -10dB points
          int index = markers[m].index;
          freq_t f = markers[m].frequency;
          uint32_t s = actual_rbw_x10 * 200;                        // twice the selected RBW
          int left = index, right = index;
          while (t > 0 && actual_t[t+1] > v && markers[t].frequency > f - s)                                        // Find left point
            t--;
          if (t > 0) {
            left = t;
          }
          t = setting._sweep_points-1;;
          while (t > setting._sweep_points-1 && actual_t[t+1] > v)                // find right -3dB point
            t++;
          if (t > index) {
            right = t;
            markers[2].frequency = frequencies[t];
          }

#endif

          interpolate_maximum(m);
          m++;
          break;                          // Next maximum
        }
        m++;                              // Try next marker
      }
      i++;
    }
    while (m < MARKERS_MAX) {                  // Insufficient maxima found
      if (markers[m].enabled && markers[m].mtype & M_TRACKING) {    // More available markers found
        markers[m].index = 0;                             // Enabled but no max so set to left most frequency
        markers[m].frequency = frequencies[0];
      }
      m++;                              // Try next marker
    }

    // ----------------------- now follow all the special marker calculations for the measurement modes ----------------------------


#ifdef __MEASURE__
    if (setting.measurement == M_IMD && markers[0].index > 10) {                    // ----- IMD measurement
      markers[1].enabled = search_maximum(1, frequencies[markers[0].index]*2, 8);
      markers[2].enabled = search_maximum(2, frequencies[markers[0].index]*3, 12);
      markers[3].enabled = search_maximum(3, frequencies[markers[0].index]*4, 16);
    } else if (setting.measurement == M_OIP3  && markers[0].index > 10 && markers[1].index > 10) { // ----------IOP measurement
      int l = markers[0].index;
      int r = markers[1].index;
      if (r < l) {
        l = markers[1].index;
        r = markers[0].index;
        markers[0].index = l;
        markers[1].index = r;
      }
      freq_t lf = frequencies[l];
      freq_t rf = frequencies[r];
      markers[0].frequency = lf;
      markers[1].frequency = rf;

      markers[2].enabled = search_maximum(2, lf - (rf - lf), 12);
      markers[3].enabled = search_maximum(3, rf + (rf - lf), 12);
    } else if (setting.measurement == M_PHASE_NOISE  && markers[0].index > 10) {    //  ------------Phase noise measurement
      markers[1].index =  markers[0].index + (setting.mode == M_LOW ? 290/4 : -290/4);  // Position phase noise marker at requested offset
      markers[1].frequency = frequencies[markers[1].index];
    } else if (setting.measurement == M_STOP_BAND  && markers[0].index > 10) {      // -------------Stop band measurement
      markers[1].index =  marker_search_left_min(markers[0].index);
      if (markers[1].index < 0) markers[1].index = 0;
      markers[1].frequency = frequencies[markers[1].index];
      markers[2].index =  marker_search_right_min(markers[0].index);
      if (markers[2].index < 0) markers[1].index = setting._sweep_points - 1;
      markers[2].frequency = frequencies[markers[2].index];
    } else if ((setting.measurement == M_PASS_BAND || setting.measurement == M_FM)  && markers[0].index > 10) {      // ----------------Pass band measurement
      int t = 0;
      float v = actual_t[markers[0].index] - 3.0;
      while (t < markers[0].index && actual_t[t+1] < v)                                        // Find left -3dB point
        t++;
      if (t< markers[0].index) {
        markers[1].index = t;
        markers[1].frequency = frequencies[t];
      }
      t = setting._sweep_points-1;;
      while (t > markers[0].index && actual_t[t-1] < v)                // find right -3dB point
        t--;
      if (t > markers[0].index) {
        markers[2].index = t;
        markers[2].frequency = frequencies[t];
      }
    } else if (setting.measurement == M_AM) {      // ----------------AM measurement
      if (S_IS_AUTO(setting.agc )) {
        int actual_level = actual_t[max_index[0]] - get_attenuation();  // no need for float
        if (actual_level > -20 ) {
          setting.agc = S_AUTO_OFF;
          setting.lna = S_AUTO_OFF;
        } else if (actual_level < -45 ) {
          setting.agc = S_AUTO_ON;
          setting.lna = S_AUTO_ON;
        } else {
          setting.agc = S_AUTO_OFF;
          setting.lna = S_AUTO_ON;
        }
        set_AGC_LNA();
      }
    }

#endif
    peakIndex = max_index[0];
    peakLevel = actual_t[peakIndex];
    peakFreq = frequencies[peakIndex];
    min_level = temp_min_level;
  }
  //  } while (MODE_OUTPUT(setting.mode) && setting.modulation != MO_NONE);      // Never exit sweep loop while in output mode with modulation
#if 0       // Read ADC
  extern   int fix_fft(short fr[], short fi[], short m, short inverse);
  extern int16_t adc_buf_read(uint32_t chsel, uint16_t *result, uint32_t count);
  trace[TRACE_STORED].enabled = true;
  adc_buf_read(ADC_CHSELR_CHSEL4, spi_buffer, 290);
#if 1           // Perform FFT on input
  int32_t zero = 0;
  for (int i=0;i<256;i++) {
    zero += spi_buffer[i];
  }
  zero = zero >> 8;
  int16_t *rfft = (int16_t *)&spi_buffer[0];
  int16_t *ifft = (int16_t *)&spi_buffer[512];
  for (int i=0;i<256;i++) {
    rfft[i] = spi_buffer[i] - zero;
    ifft[i] = rfft[i];    // Imaginary part equal to real part
    rfft[511 - i] = rfft[i];    // Mirror real
    ifft[511 - i] = -rfft[i]; // Conjugate mirror for imaginary part
  }
  fix_fft(rfft,ifft, 9,false);
#endif
  for (int i=0;i<256;i++) {             // Concert to
#if 1                                   // Linear
    stored_t[i] = (((int16_t *)spi_buffer)[i]/44.0) - 80.0;
#else
    float r = rfft[i];                  // Log
    if (r < 0)
      r = -r;
    float im = ifft[i];
    if (im < 0)
      im = -im;
    if (r == 0)
      r = 1;
    if (im==0)
      im = 1;
    stored_t[i] = (log10(r) * 2.0 + log10(im) * 2.0)/2.0 - 80.0;
#endif
  }
#endif


#ifdef __LINEARITY__
  //---------------- in Linearity measurement the attenuation has to be adapted ------------------
  if (setting.measurement == M_LINEARITY && setting.linearity_step < sweep_points) {
    setting.attenuate_x2 = (29.0 - setting.linearity_step * 30.0 / (sweep_points))*2.0;
    dirty = true;
    stored_t[setting.linearity_step] = peakLevel;
    setting.linearity_step++;
  }
#endif
  //    redraw_marker(peak_marker, FALSE);
  //  STOP_PROFILE;
#ifdef TINYSA3
  palSetPad(GPIOB, GPIOB_LED);
#endif
#ifdef TINYSA4
  palSetLine(LINE_LED);
#endif
  
  return true;
}

//------------------------------- SEARCH ---------------------------------------------

int
marker_search_left_max(int from)
{
  int i;
  int found = -1;
  if (uistat.current_trace == -1)
    return -1;

  float value = actual_t[from];
  for (i = from - 1; i >= 0; i--) {
    float new_value = actual_t[i];
    if (new_value < value) {
      value = new_value;
      found = i;
    } else if (new_value > value + setting.noise )
      break;
  }

  for (; i >= 0; i--) {
    float new_value = actual_t[i];
    if (new_value > value) {
      value = new_value;
      found = i;
    } else if (new_value < value  - setting.noise )
      break;
  }
  return found;
}

int
marker_search_right_max(int from)
{
  int i;
  int found = -1;

  if (uistat.current_trace == -1)
    return -1;
  float value = actual_t[from];
  for (i = from + 1; i < sweep_points; i++) {
    float new_value = actual_t[i];
    if (new_value < value) {    // follow down
      value = new_value;
      found = i;
    } else if (new_value > value + setting.noise) // larger then lowest value + noise
      break;    //  past the minimum
  }
  for (; i < sweep_points; i++) {
    float new_value = actual_t[i];
    if (new_value > value) {    // follow up
      value = new_value;
      found = i;
    } else if (new_value < value - setting.noise)
      break;
  }
  return found;
}

int marker_search_max(void)
{
  int i = 0;
  int found = 0;

  float value = actual_t[i];
  for (; i < sweep_points; i++) {
    int new_value = actual_t[i];
    if (new_value > value) {    // follow up
      value = new_value;
      found = i;
    }
  }
  return found;
}

#define MINMAX_DELTA_X10 100


int
marker_search_left_min(int from)
{
  int i;
  int found = from;
  if (uistat.current_trace == -1)
    return -1;

  int value_x10 = actual_t[from]*10;
  for (i = from - 1; i >= 0; i--) {
    int new_value_x10 = actual_t[i]*10;
    if (new_value_x10 > value_x10) {
      value_x10 = new_value_x10;        // follow up
//      found = i;
    } else if (new_value_x10 < value_x10 - MINMAX_DELTA_X10 )
      break;  // past the maximum
  }

  for (; i >= 0; i--) {
    int new_value_x10 = actual_t[i]*10;
    if (new_value_x10 < value_x10) {
      value_x10 = new_value_x10;        // follow down
      found = i;
    } else if (new_value_x10 > value_x10  + MINMAX_DELTA_X10 )
      break;
  }
  return found;
}

int
marker_search_right_min(int from)
{
  int i;
  int found = from;

  if (uistat.current_trace == -1)
    return -1;
  int value_x10 = actual_t[from]*10;
  for (i = from + 1; i < sweep_points; i++) {
    int new_value_x10 = actual_t[i]*10;
    if (new_value_x10 > value_x10) {    // follow up
      value_x10 = new_value_x10;
//      found = i;
    } else if (new_value_x10 < value_x10 - MINMAX_DELTA_X10) // less then largest value_x10 - noise
      break;    // past the maximum
  }
  for (; i < sweep_points; i++) {
    int new_value_x10 = actual_t[i]*10;
    if (new_value_x10 < value_x10) {    // follow down
      value_x10 = new_value_x10;
      found = i;
    } else if (new_value_x10 > value_x10 + MINMAX_DELTA_X10) // larger then smallest value_x10 + noise
      break;
  }
  return found;
}




// -------------------- Self testing -------------------------------------------------

enum {
  TC_SIGNAL, TC_BELOW, TC_ABOVE, TC_FLAT, TC_MEASURE, TC_SET, TC_END, TC_ATTEN, TC_DISPLAY,
};

enum {
  TP_SILENT, TPH_SILENT, TP_10MHZ, TP_10MHZEXTRA, TP_10MHZ_SWITCH, TP_30MHZ, TPH_30MHZ, TPH_30MHZ_SWITCH
};

#define TEST_COUNT  (sizeof test_case / sizeof test_case[0])

#define W2P(w) (sweep_points * w / 100)     // convert width in % to actual sweep points

typedef struct test_case {
  int kind;
  int setup;
  float center;      // In MHz
  float span;        // In MHz
  float pass;
  int width;
  float stop;
} test_case_t;

const test_case_t test_case [] =
{// Condition   Preparation     Center  Span    Pass    Width(%)Stop
 {TC_BELOW,     TP_SILENT,      0.005,  0.01,   0,      0,      0},         // 1 Zero Hz leakage
 {TC_BELOW,     TP_SILENT,      0.015,   0.01,   -30,    0,      0},         // 2 Phase noise of zero Hz
 {TC_SIGNAL,    TP_10MHZ,       20,     7,      -39,    10,     -90 },      // 3
 {TC_SIGNAL,    TP_10MHZ,       30,     7,      -34,    10,     -90 },      // 4
#define TEST_SILENCE 4
 {TC_BELOW,     TP_SILENT,      200,    100,    -75,    0,      0},         // 5  Wide band noise floor low mode
 {TC_BELOW,     TPH_SILENT,     600,    720,    -75,    0,      0},         // 6 Wide band noise floor high mode
 {TC_SIGNAL,    TP_10MHZEXTRA,  10,     7,      -20,    27,     -80 },      // 7 BPF loss and stop band
 {TC_FLAT,      TP_10MHZEXTRA,  10,     4,      -18,    9,     -60},       // 8 BPF pass band flatness
 {TC_BELOW,     TP_30MHZ,       400,    60,     -75,    0,      -75},       // 9 LPF cutoff
 {TC_SIGNAL,    TP_10MHZ_SWITCH,20,     7,      -39,    10,     -60 },      // 10 Switch isolation using high attenuation
 {TC_DISPLAY,     TP_30MHZ,       30,     0,      -25,    145,     -60 },      // 11 Measure atten step accuracy
 {TC_ATTEN,     TP_30MHZ,       30,     0,      -25,    145,     -60 },      // 12 Measure atten step accuracy
#define TEST_END 12
 {TC_END,       0,              0,      0,      0,      0,      0},
#define TEST_POWER  13
 {TC_MEASURE,   TP_30MHZ,       30,     7,      -25,   10,     -55 },      // 12 Measure power level and noise
 {TC_MEASURE,   TP_30MHZ,       270,    4,      -50,    10,     -75 },       // 13 Measure powerlevel and noise
 {TC_MEASURE,   TPH_30MHZ,      270,    4,      -40,    10,     -65 },       // 14 Calibrate power high mode
 {TC_END,       0,              0,      0,      0,      0,      0},
#define TEST_RBW    17
 {TC_MEASURE,   TP_30MHZ,       30,     1,      -20,    10,     -60 },      // 16 Measure RBW step time
 {TC_END,       0,              0,      0,      0,      0,      0},
 {TC_MEASURE,   TPH_30MHZ,      300,    4,      -48,    10,     -65 },       // 14 Calibrate power high mode
 {TC_MEASURE,   TPH_30MHZ_SWITCH,300,    4,      -40,    10,     -65 },       // 14 Calibrate power high mode
#define TEST_ATTEN    21
 {TC_ATTEN,      TP_30MHZ,       30,     0,      -25,    145,     -60 },      // 20 Measure atten step accuracy
#define TEST_SPUR    22
 {TC_BELOW,      TP_SILENT,     96,     8,      -95,    0,     0 },       // 22 Measure 48MHz spur
};



enum {
  TS_WAITING, TS_PASS, TS_FAIL, TS_CRITICAL
};
static const  char *(test_text [4]) =
{
 "Waiting", "Pass", "Fail", "Critical"
};

static const  char *(test_fail_cause [TEST_COUNT]);
static int test_status[TEST_COUNT];
static int show_test_info = FALSE;
static volatile int test_wait = false;
static float test_value;

static void test_acquire(int i)
{
  (void)i;
  pause_sweep();
//  SetAverage(4);
  sweep(false);
//  sweep(false);
//  sweep(false);
//  sweep(false);
  plot_into_index(measured);
  redraw_request |= REDRAW_CELLS | REDRAW_FREQUENCY;
}

void cell_drawstring(char *str, int x, int y);

static char self_test_status_buf[35];
void cell_draw_test_info(int x0, int y0)
{
#define INFO_SPACING    13
//  char self_test_status_buf[35];
  if (!show_test_info)
    return;
  int i = -2;
  do {
    i++;
    int xpos = 25 - x0;
    int ypos = 50+i*INFO_SPACING - y0;
    unsigned int color = LCD_FG_COLOR;
    if (i == -1) {
        plot_printf(self_test_status_buf, sizeof self_test_status_buf, "Self test status:");
    } else if (test_case[i].kind == TC_END) {
        if (test_wait)
          plot_printf(self_test_status_buf, sizeof self_test_status_buf, "Touch screen to continue");
        else
          self_test_status_buf[0] = 0;
      } else {
      plot_printf(self_test_status_buf, sizeof self_test_status_buf, "Test %d: %s%s", i+1, test_fail_cause[i], test_text[test_status[i]] );
      if (test_status[i] == TS_PASS)
        color = LCD_BRIGHT_COLOR_GREEN;
      else if (test_status[i] == TS_CRITICAL)
        color = LCD_TRACE_3_COLOR;          // Yellow
      else if (test_status[i] == TS_FAIL)
        color = LCD_BRIGHT_COLOR_RED;
      else
        color = LCD_BRIGHT_COLOR_BLUE;
    }
    ili9341_set_foreground(color);
    cell_drawstring(self_test_status_buf, xpos, ypos);
  } while (test_case[i].kind != TC_END);
}

int validate_signal_within(int i, float margin)
{
  test_fail_cause[i] = "Signal level ";
  if (fabsf(peakLevel-test_case[i].pass) > 2*margin) {
    return TS_FAIL;
  }
  if (fabsf(peakLevel-test_case[i].pass) > margin) {
    return TS_CRITICAL;
  }
  if (setting.measurement == M_PASS_BAND) {
    peakFreq = (markers[2].frequency + markers[1].frequency)/2;
    markers[0].frequency = peakFreq;
  }
  test_fail_cause[i] = "Frequency ";
  if (peakFreq < test_case[i].center * 1000000 - 100000 || test_case[i].center * 1000000 + 100000 < peakFreq )
    return TS_FAIL;
  test_fail_cause[i] = "";
  return TS_PASS;
}

int validate_peak_below(int i, float margin) {
  return(test_case[i].pass - peakLevel > margin);
}

int validate_below(int tc, int from, int to) {
  int status = TS_PASS;
  float threshold=stored_t[from];
  float sum = 0;
  int sum_count = 0;
  for (int j = from; j < to; j++) {
    sum += actual_t[j];
    sum_count++;
    if (actual_t[j] > threshold) {
      status = TS_FAIL;
      break;
    }
  }
  sum = sum / sum_count;
  if (sum > threshold - 5)
    status = TS_CRITICAL;
  if (status != TS_PASS)
    test_fail_cause[tc] = "Above ";
  return(status);
}

int validate_flatness(int i) {
  volatile int j,k;
  test_fail_cause[i] = "Passband ";
  for (j = peakIndex; j < setting._sweep_points; j++) {
    if (actual_t[j] < peakLevel - 15)    // Search right -3dB
      break;
  }
  for (k = peakIndex; k > 0; k--) {
    if (actual_t[k] < peakLevel - 15)    // Search left -3dB
      break;
  }
//  shell_printf("Width %d between %d and %d\n\r", j - k, 2* W2P(test_case[i].width), 3* W2P(test_case[i].width) );
  if (j - k < 2* W2P(test_case[i].width))
      return(TS_FAIL);
  if (j - k > 3* W2P(test_case[i].width))
      return(TS_FAIL);
  test_fail_cause[i] = "";
  return(TS_PASS);
}


const float atten_step[7] = { 0.0, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0 };

int validate_atten(int i) {
  int status = TS_PASS;
  float reference_peak_level = 0.0;
  test_fail_cause[i] = "Attenuator ";
//  for (int j= 0; j < 64; j++ ) {
  for (int j= 0; j < 7; j++ ) {
//    float a = ((float)j)/2.0;
    float a = atten_step[j];
    set_attenuation(a);
    float summed_peak_level = 0;
#define ATTEN_TEST_SWEEPS    5
    for (int k=0; k<ATTEN_TEST_SWEEPS; k++) {
//        setting.sweep_time_us = 1000000;
        test_acquire(TEST_ATTEN);                        // Acquire test
//      test_validate(TEST_ATTEN);                       // Validate test
        float peaklevel = 0.0;
      for (int n = 0 ; n < sweep_points; n++)
        peaklevel += actual_t[n];
      peaklevel /= (sweep_points - 0);
      summed_peak_level += peaklevel;
      }
      summed_peak_level /= ATTEN_TEST_SWEEPS;
    if (j == 0)
      reference_peak_level = summed_peak_level;
    else {
//      if (SDU1.config->usbp->state == USB_ACTIVE)  shell_printf("Attenuation %.2fdB, measured level %.2fdBm, delta %.2fdB\n\r",a, summed_peak_level, summed_peak_level - reference_peak_level);
#define ATTEN_TEST_CRITERIA 1
      if (summed_peak_level - reference_peak_level <= -ATTEN_TEST_CRITERIA || summed_peak_level - reference_peak_level >= ATTEN_TEST_CRITERIA) {
        status = TS_FAIL;
 //       draw_all(true);
      }

    }
  }
  if (status == TS_PASS)
    test_fail_cause[i] = "";
  return(status);
}

int validate_display(int tc)
{
  test_fail_cause[tc] = "Display ";
  if (!display_test()) {
    return(TS_FAIL);
  }
  test_fail_cause[tc] = "";
  return(TS_PASS);
}

int validate_above(int tc) {
  int status = TS_PASS;
  for (int j = 0; j < setting._sweep_points; j++) {
    if (actual_t[j] < stored_t[j] + 5)
      status = TS_CRITICAL;
    else if (actual_t[j] < stored_t[j]) {
      status = TS_FAIL;
      break;
    }
  }
  if (status != TS_PASS)
    test_fail_cause[tc] = "Below ";
  return(status);
}


int test_validate(int i)
{
//  draw_all(TRUE);
  int current_test_status = TS_PASS;
  switch (test_case[i].kind) {
  case TC_SET:
    if (test_case[i].pass == 0) {
      if (test_value != 0)
        set_actual_power(test_value);
    } else
      set_actual_power(test_case[i].pass);
    goto common;
  case TC_MEASURE:
  case TC_SIGNAL:           // Validate signal
  common: current_test_status = validate_signal_within(i, 10.0);
    if (current_test_status == TS_PASS) {            // Validate noise floor
      current_test_status = validate_below(i, 0, setting._sweep_points/2 - W2P(test_case[i].width));
      if (current_test_status == TS_PASS) {
        current_test_status = validate_below(i, setting._sweep_points/2 + W2P(test_case[i].width), setting._sweep_points);
      }
      if (current_test_status != TS_PASS)
        test_fail_cause[i] = "Stopband ";
    }
    if (current_test_status == TS_PASS && test_case[i].kind == TC_MEASURE)
      test_value = peakLevel;
    else
      test_value = 0;           //   Not valid
  break;
  case TC_ABOVE:   // Validate signal above curve
    current_test_status = validate_above(i);
    break;
  case TC_BELOW:   // Validate signal below curve
    current_test_status = validate_below(i, 0, setting._sweep_points);
    break;
  case TC_FLAT:   // Validate passband flatness
    current_test_status = validate_flatness(i);
    break;
  case TC_ATTEN:
    current_test_status = validate_atten(i);
    break;
  case TC_DISPLAY:
    current_test_status = validate_display(i);
    break;
  }

  // Report status

  if (current_test_status != TS_PASS || test_case[i+1].kind == TC_END)
    test_wait = true;
  test_status[i] = current_test_status;     // Must be set before draw_all() !!!!!!!!
  //  draw_frequencies();
//  draw_cal_status();
  draw_all(TRUE);
  return current_test_status;
}

void test_prepare(int i)
{
  setting.tracking = false; //Default test setup
  setting.atten_step = false;
  setting.frequency_IF = DEFAULT_IF;                // Default frequency
  setting.auto_IF = true;
  setting.auto_attenuation = false;
  setting.spur_removal = S_OFF;

  switch(test_case[i].setup) {                // Prepare test conditions
  case TPH_SILENT:                             // No input signal
    set_mode(M_HIGH);
    goto common_silent;
  case TP_SILENT:                             // No input signal
    set_mode(M_LOW);
common_silent:
    set_refer_output(-1);
    for (int j = 0; j < setting._sweep_points; j++)
      stored_t[j] = test_case[i].pass;
    break;
  case TP_10MHZ_SWITCH:
    set_mode(M_LOW);
    set_refer_output(2);
    goto common;
  case TP_10MHZEXTRA:                         // Swept receiver
    set_mode(M_LOW);
    setting.tracking = true; //Sweep BPF
    setting.auto_IF = false;
    setting.frequency_IF = DEFAULT_IF+210000;                // Center on SAW filters
    set_refer_output(2);
    markers[1].enabled = M_ENABLED;
    markers[1].mtype = M_DELTA;
    markers[2].enabled = M_ENABLED;
    markers[2].mtype = M_DELTA;
    setting.measurement = M_PASS_BAND;
    goto common;
  case TP_10MHZ:                              // 10MHz input
    set_mode(M_LOW);
    set_refer_output(2);
    setting.step_delay_mode = SD_PRECISE;
//        set_step_delay(1);                      // Precise scanning speed
#ifdef __SPUR__
#ifdef TINYSA4
    setting.spur_removal = S_AUTO_OFF;
#else
    setting.spur_removal = S_ON;
#endif
#endif
 common:

    for (int j = 0; j < setting._sweep_points/2 - W2P(test_case[i].width); j++)
      stored_t[j] = test_case[i].stop;
    for (int j = setting._sweep_points/2 + W2P(test_case[i].width); j < setting._sweep_points; j++)
      stored_t[j] = test_case[i].stop - (i == 6?5:0);
    for (int j = setting._sweep_points/2 - W2P(test_case[i].width); j < setting._sweep_points/2 + W2P(test_case[i].width); j++)
      stored_t[j] = test_case[i].pass;
    break;
  case TP_30MHZ:
    set_mode(M_LOW);
    maxFreq = 520000000;            // needed to measure the LPF rejection
    set_refer_output(0);
    dirty = true;
 //   set_step_delay(1);                      // Do not set !!!!!
#ifdef __SPUR__
    setting.spur_removal = S_ON;
#endif

    goto common;
  case TPH_30MHZ_SWITCH:
  case TPH_30MHZ:
    set_mode(M_HIGH);
    set_refer_output(0);
    setting.spur_removal = S_ON;
    goto common;
  }
  switch(test_case[i].setup) {                // Prepare test conditions
  case TP_10MHZ_SWITCH:
    set_attenuation(32);                        // This forces the switch to transmit so isolation can be tested
    break;
  case TPH_30MHZ_SWITCH:
    set_attenuation(0);
    setting.atten_step = true;               // test high switch isolation
    break;
  default:
    set_attenuation(0.0);
  }
  trace[TRACE_STORED].enabled = true;
  set_reflevel(test_case[i].pass+10);
  set_sweep_frequency(ST_CENTER, (freq_t)(test_case[i].center * 1000000));
  set_sweep_frequency(ST_SPAN, (freq_t)(test_case[i].span * 1000000));
  draw_cal_status();
}

extern void menu_autosettings_cb(int item);


int last_spur = 0;
int add_spur(int f)
{
  for (int i = 0; i < last_spur; i++) {
    if (temp_t[i] == f) {
      stored_t[i] += 1;
      return stored_t[i];
    }
  }
  if (last_spur < POINTS_COUNT) {
    temp_t[last_spur] = f;
    stored_t[last_spur++] = 1;
  }
  return 1;
}

//static bool test_wait = false;
static int test_step = 0;

void self_test(int test)
{
//  set_sweep_points(POINTS_COUNT);
  if (test == 0) {
    if (test_wait ) {
      if (test_case[test_step].kind == TC_END || setting.test_argument != 0)
        goto resume2;
      else
        goto resume;
    }
    reset_settings(M_LOW);                      // Make sure we are in a defined state
    in_selftest = true;
    menu_autosettings_cb(0);
    for (uint16_t i=0; i < TEST_COUNT; i++) {          // All test cases waiting
      if (test_case[i].kind == TC_END)
        break;
      test_status[i] = TS_WAITING;
      test_fail_cause[i] = "";
    }
    show_test_info = TRUE;
    test_step=0;
    if (setting.test_argument > 0)
      test_step=setting.test_argument-1;
    do {
      test_prepare(test_step);
      test_acquire(test_step);                        // Acquire test
      test_status[test_step] = test_validate(test_step);                       // Validate test
      if (test_step == 2) {
        if (peakLevel < -60) {
          test_step = TEST_END;
          ili9341_set_foreground(LCD_BRIGHT_COLOR_RED);
          ili9341_drawstring_7x13("Signal level too low", 30, 140);
          ili9341_drawstring_7x13("Did you connect high and low ports with cable?", 0, 210);
          goto resume2;
        }

      }
      if (test_status[test_step] != TS_PASS) {
        resume:
        test_wait = true;
        if (!check_touched())
          return;
//        wait_user();
      }
      test_step++;
    } while (test_case[test_step].kind != TC_END && setting.test_argument == 0 );
    ili9341_set_foreground(LCD_BRIGHT_COLOR_GREEN);
    ili9341_drawstring_7x13("Self test complete", 50, 200);
    ili9341_drawstring_7x13("Touch screen to continue", 50, 215);
   resume2:
    test_wait = true;
    if (!check_touched())
      return;
    sweep_mode = SWEEP_ENABLE;

    ili9341_clear_screen();
    reset_settings(M_LOW);
    set_refer_output(-1);
#ifdef DOESNOTFIT
  } else if (test == 1) {
    float p2, p1, p;
    in_selftest = true;               // Spur search
    reset_settings(M_LOW);
    test_prepare(TEST_SILENCE);
    setting.auto_IF = false;
    setting.frequency_IF=DEFAULT_IF;
    setting.frequency_step = 30000;
    if (setting.test_argument > 0)
      setting.frequency_step=setting.test_argument;
    freq_t f = 400000;           // Start search at 400kHz
    //  int i = 0;                     // Index in spur table (temp_t)
    set_RBW(setting.frequency_step/100);
    last_spur = 0;
    for (int j = 0; j < 4; j++) {

      p2 = PURE_TO_float(perform(false, 0, f, false));
      vbwSteps = 1;
      f += setting.frequency_step;
      p1 = PURE_TO_float(perform(false, 1, f, false));
      f += setting.frequency_step;
      shell_printf("\n\rStarting with %4.2f, %4.2f and IF at %d and step of %d\n\r", p2, p1, setting.frequency_IF, setting.frequency_step );
      f = 400000;
      while (f < DEFAULT_MAX_FREQ) {
        p = PURE_TO_float(perform(false, 1, f, false));
#define SPUR_DELTA  6
        if ( p2 < p1 - SPUR_DELTA  && p < p1 - SPUR_DELTA) {
          shell_printf("Spur of %4.2f at %d with count %d\n\r", p1,(f - setting.frequency_step)/1000, add_spur(f - setting.frequency_step));
        }
        p2 = p1;
        p1 = p;
        f += setting.frequency_step;
      }
    }
    shell_printf("\n\rTable for IF at %d and step of %d\n\r", setting.frequency_IF, setting.frequency_step);
    for (int j = 0; j < last_spur; j++) {
      if ((int)stored_t[j] > 1)
        shell_printf("%d, %d\n\r", ((int)temp_t[j])/1000, (int)stored_t[j]);
    }
    reset_settings(M_LOW);
  } else if (test == 2) {                                   // Attenuator test
    in_selftest = true;
    reset_settings(M_LOW);
    float reference_peak_level = 0;
    test_prepare(TEST_ATTEN);
    test_acquire(TEST_ATTEN);                        // Acquire test
    test_validate(TEST_ATTEN);                       // Validate test
#if 0
    for (int j= 0; j < 64; j++ ) {
//      test_prepare(TEST_ATTEN);
      set_attenuation(((float)j)/2.0);
      float summed_peak_level = 0;
//      for (int k=0; k<10; k++) {
        test_acquire(TEST_ATTEN);                        // Acquire test
        test_validate(TEST_ATTEN);                       // Validate test
//        summed_peak_level += peakLevel;
//      }
        float peaklevel = 0.0;
        for (int k = 0 ; k < sweep_points; k++)
          peaklevel += actual_t[k];
        peaklevel /= sweep_points;
      if (j == 0)
        reference_peak_level = peakLevel;
      shell_printf("Attenuation %.2fdB, measured level %.2fdBm, delta %.2fdB\n\r",((float)j)/2.0, peakLevel, peakLevel - reference_peak_level);
    }
#endif
    reset_settings(M_LOW);
  } else if (test == 3) {                       // RBW step time search
    in_selftest = true;
//    reset_settings(M_LOW);
    setting.auto_IF = false;
    setting.frequency_IF=DEFAULT_IF;
    ui_mode_normal();
    test_prepare(TEST_RBW);
    setting.step_delay = 8000;
    for (int j= 0; j < SI4432_RBW_count; j++ ) {
      if (setting.test_argument != 0)
        j = setting.test_argument;
// do_again:
      test_prepare(TEST_RBW);
      setting.spur_removal = S_OFF;
#if 1               // Disable for offset baseline scanning
      setting.step_delay_mode = SD_NORMAL;
      setting.repeat = 1;
#else
      setting.step_delay_mode = SD_FAST;
      setting.repeat = 20;
#endif
      setting.step_delay = setting.step_delay * 5 / 4;
      setting.offset_delay = setting.step_delay / 2;
      setting.rbw_x10 = force_rbw(j);

      shell_printf("RBW = %f, ",setting.rbw_x10/10.0);
#if 0
      set_sweep_frequency(ST_SPAN, (freq_t)(setting.rbw_x10 * 1000));     // Wide
#else
      if (setting.rbw_x10 < 1000)
        set_sweep_frequency(ST_SPAN, (freq_t)(setting.rbw_x10 * 5000));   // Narrow
      else
        set_sweep_frequency(ST_SPAN, (freq_t)(18000000));
#endif
      test_acquire(TEST_RBW);                        // Acquire test
      test_validate(TEST_RBW);                       // Validate test
//      if (test_value == 0) {
//        setting.step_delay = setting.step_delay * 4 / 5;
//        goto do_again;
//      }

      float saved_peakLevel = peakLevel;
 //     if (peakLevel < -35) {
 //       shell_printf("Peak level too low, abort\n\r");
 //       return;
 //     }
      shell_printf("Start level = %f, ",peakLevel);
#if 1                                                                       // Enable for step delay tuning
      while (setting.step_delay > 10 && test_value != 0 && test_value > saved_peakLevel - 0.5) {
        test_prepare(TEST_RBW);
        setting.spur_removal = S_OFF;
        setting.step_delay_mode = SD_NORMAL;
        setting.step_delay = setting.step_delay * 4 / 5;
        if (setting.rbw_x10 < 1000)
          set_sweep_frequency(ST_SPAN, (freq_t)(setting.rbw_x10 * 5000));
        else
          set_sweep_frequency(ST_SPAN, (freq_t)(18000000));

//        setting.repeat = 10;
        test_acquire(TEST_RBW);                        // Acquire test
        test_validate(TEST_RBW);                       // Validate test
        //      shell_printf(" Step %f, %d",peakLevel, setting.step_delay);
      }

      setting.step_delay = setting.step_delay * 5 / 4;          // back one level
#else
      setting.step_delay = setting.step_delay * 4 / 5;

#endif
      setting.offset_delay = 1600;
#if 1                       // Enable for offset tuning stepping
      test_value = saved_peakLevel;
      if ((uint32_t)(setting.rbw_x10 * 1000) / (sweep_points) < 8000) {           // fast mode possible
        while (setting.offset_delay > 0 && test_value != 0 && test_value > saved_peakLevel - 1.5) {
          test_prepare(TEST_RBW);
          setting.step_delay_mode = SD_FAST;
          setting.offset_delay /= 2;
          setting.spur_removal = S_OFF;
          if (setting.rbw_x10 < 1000)
            set_sweep_frequency(ST_SPAN, (freq_t)(setting.rbw_x10 * 5000));   // 50 times RBW
          else
            set_sweep_frequency(ST_SPAN, (freq_t)(18000000));     // Limit to 18MHz
//          setting.repeat = 10;
          test_acquire(TEST_RBW);                        // Acquire test
          test_validate(TEST_RBW);                       // Validate test
          //      shell_printf(" Step %f, %d",peakLevel, setting.step_delay);
        }
      }
#endif
      shell_printf("End level = %f, step time = %d, fast delay = %d\n\r",peakLevel, setting.step_delay, setting.offset_delay*2);
      if (setting.test_argument != 0)
        break;
    }
    reset_settings(M_LOW);
    setting.step_delay_mode = SD_NORMAL;
    setting.step_delay = 0;
  } else if (test == 5) {
//    reset_settings(M_LOW);                      // Make sure we are in a defined state
    in_selftest = true;
    switch (setting.test_argument) {
    case 0:
      touch_draw_test();
      area_width  = AREA_WIDTH_NORMAL;
      area_height = AREA_HEIGHT_NORMAL;
      break;
    case 1:
      reset_settings(M_LOW);
      set_sweep_frequency(ST_START, 0);
      set_sweep_frequency(ST_STOP, 50000000);
      break;
    case 2:
      reset_settings(M_LOW);
      set_sweep_frequency(ST_START, 300000000);
      set_sweep_frequency(ST_STOP, DEFAULT_MAX_FREQ);
      break;
    case 3:
      reset_settings(M_HIGH);
      set_sweep_frequency(ST_START, 300000000);
      set_sweep_frequency(ST_STOP, DEFAULT_MAX_FREQ);
      break;
    case 4:
      reset_settings(M_GENLOW);
      set_sweep_frequency(ST_CENTER, 20000000);
      set_sweep_frequency(ST_SPAN, 0);
      setting.mute = false;
      break;
    case 5:
      reset_settings(M_GENHIGH);
      set_sweep_frequency(ST_CENTER, 320000000);
      set_sweep_frequency(ST_SPAN, 0);
      break;
    }
  } else if (test == 6) {
    in_selftest = true;               // Spur search
    reset_settings(M_LOW);
    test_prepare(TEST_SPUR);
    for (int i = 0; i < 31; i++) {
      hsical = (RCC->CR & 0xff00) >> 8;
      RCC->CR &= RCC_CR_HSICAL;
      RCC->CR |= ( (hsical) << 8 );
      RCC->CR &= RCC_CR_HSITRIM | RCC_CR_HSION; /* CR Reset value.              */
      RCC->CR |= (i << 3 ) & RCC_CR_HSITRIM;
//      set_RBW(100);
      test_acquire(TEST_SPUR);                        // Acquire test
      shell_printf("%d: %9.3q\n\r",i, peakFreq);
      test_validate(TEST_SPUR);                       // Validate test
    }
#endif
  }

  show_test_info = FALSE;
  in_selftest = false;
  test_wait = false;
  sweep_mode = SWEEP_ENABLE;
}

void reset_calibration(void)
{
  config.high_level_offset = 100;
  config.low_level_offset = 100;
}

#define CALIBRATE_RBWS  1
const int power_rbw [5] = { 100, 300, 30, 10, 3 };

void calibrate(void)
{
#ifdef __CALIBRATE__
  int local_test_status;
  int old_sweep_points = setting._sweep_points;
  in_selftest = true;
  reset_calibration();
  reset_settings(M_LOW);
  for (int j= 0; j < CALIBRATE_RBWS; j++ ) {
//    set_RBW(power_rbw[j]);
//    set_sweep_points(21);
    test_prepare(TEST_POWER);
    setting.step_delay_mode = SD_PRECISE;
    setting.agc = S_OFF;
    setting.lna = S_OFF;
    test_acquire(TEST_POWER);                        // Acquire test
    local_test_status = test_validate(TEST_POWER);                       // Validate test
    if (peakLevel < -50) {
      ili9341_set_foreground(LCD_BRIGHT_COLOR_RED);
      ili9341_drawstring_7x13("Signal level too low", 30, 140);
      ili9341_drawstring_7x13("Check cable between High and Low connectors", 30, 160);
      goto quit;
    }
//    chThdSleepMilliseconds(1000);
    if (local_test_status != TS_PASS) {
      ili9341_set_foreground(LCD_BRIGHT_COLOR_RED);
      ili9341_drawstring_7x13("Calibration failed", 30, 140);
      goto quit;
    } else {
      set_actual_power(-25.0);           // Should be -23.5dBm (V0.2) OR 25 (V0.3)
      chThdSleepMilliseconds(1000);
    }
  }
#if 0               // No high input calibration as CAL OUTPUT is unreliable

  set_RBW(100);
  test_prepare(TEST_POWER+1);
  test_acquire(TEST_POWER+1);                        // Acquire test
  float last_peak_level = peakLevel;
  local_test_status = test_validate(TEST_POWER+1);                       // Validate test
  chThdSleepMilliseconds(1000);

  config.high_level_offset = 0;           /// Preliminary setting

  for (int j = 0; j < CALIBRATE_RBWS; j++) {
    set_RBW(power_rbw[j]);
    test_prepare(TEST_POWER+2);
    test_acquire(TEST_POWER+2);                        // Acquire test
    local_test_status = test_validate(TEST_POWER+2);                       // Validate test
//    if (local_test_status != TS_PASS) {                       // Do not validate due to variations in SI4432
//      ili9341_set_foreground(BRIGHT_COLOR_RED);
//      ili9341_drawstring_7x13("Calibration failed", 30, 120);
//      goto quit;
//    } else
      set_actual_power(last_peak_level);
      chThdSleepMilliseconds(1000);
  }

#endif

  config_save();
  ili9341_set_foreground(LCD_BRIGHT_COLOR_GREEN);
  ili9341_drawstring_7x13("Calibration complete", 30, 140);
quit:
  ili9341_drawstring_7x13("Touch screen to continue", 30, 200);
  wait_user();
  ili9341_clear_screen();
  set_sweep_points(old_sweep_points);

  in_selftest = false;
  sweep_mode = SWEEP_ENABLE;
  set_refer_output(-1);
  reset_settings(M_LOW);
#endif
}

#pragma GCC pop_options

#if 0                           // fixed point FFT

/* fix_fft.c - Fixed-point in-place Fast Fourier Transform  */
/*
  All data are fixed-point short integers, in which -32768
  to +32768 represent -1.0 to +1.0 respectively. Integer
  arithmetic is used for speed, instead of the more natural
  floating-point.

  For the forward FFT (time -> freq), fixed scaling is
  performed to prevent arithmetic overflow, and to map a 0dB
  sine/cosine wave (i.e. amplitude = 32767) to two -6dB freq
  coefficients. The return value is always 0.

  For the inverse FFT (freq -> time), fixed scaling cannot be
  done, as two 0dB coefficients would sum to a peak amplitude
  of 64K, overflowing the 32k range of the fixed-point integers.
  Thus, the fix_fft() routine performs variable scaling, and
  returns a value which is the number of bits LEFT by which
  the output must be shifted to get the actual amplitude
  (i.e. if fix_fft() returns 3, each value of fr[] and fi[]
  must be multiplied by 8 (2**3) for proper scaling.
  Clearly, this cannot be done within fixed-point short
  integers. In practice, if the result is to be used as a
  filter, the scale_shift can usually be ignored, as the
  result will be approximately correctly normalized as is.

  Written by:  Tom Roberts  11/8/89
  Made portable:  Malcolm Slaney 12/15/94 malcolm@interval.com
  Enhanced:  Dimitrios P. Bouras  14 Jun 2006 dbouras@ieee.org
*/

#define N_WAVE      1024    /* full length of Sinewave[] */
#define LOG2_N_WAVE 10      /* log2(N_WAVE) */

/*
  Henceforth "short" implies 16-bit word. If this is not
  the case in your architecture, please replace "short"
  with a type definition which *is* a 16-bit word.
*/

/*
  Since we only use 3/4 of N_WAVE, we define only
  this many samples, in order to conserve data space.
*/
short Sinewave[N_WAVE-N_WAVE/4] = {
      0,    201,    402,    603,    804,   1005,   1206,   1406,
   1607,   1808,   2009,   2209,   2410,   2610,   2811,   3011,
   3211,   3411,   3611,   3811,   4011,   4210,   4409,   4608,
   4807,   5006,   5205,   5403,   5601,   5799,   5997,   6195,
   6392,   6589,   6786,   6982,   7179,   7375,   7571,   7766,
   7961,   8156,   8351,   8545,   8739,   8932,   9126,   9319,
   9511,   9703,   9895,  10087,  10278,  10469,  10659,  10849,
  11038,  11227,  11416,  11604,  11792,  11980,  12166,  12353,
  12539,  12724,  12909,  13094,  13278,  13462,  13645,  13827,
  14009,  14191,  14372,  14552,  14732,  14911,  15090,  15268,
  15446,  15623,  15799,  15975,  16150,  16325,  16499,  16672,
  16845,  17017,  17189,  17360,  17530,  17699,  17868,  18036,
  18204,  18371,  18537,  18702,  18867,  19031,  19194,  19357,
  19519,  19680,  19840,  20000,  20159,  20317,  20474,  20631,
  20787,  20942,  21096,  21249,  21402,  21554,  21705,  21855,
  22004,  22153,  22301,  22448,  22594,  22739,  22883,  23027,
  23169,  23311,  23452,  23592,  23731,  23869,  24006,  24143,
  24278,  24413,  24546,  24679,  24811,  24942,  25072,  25201,
  25329,  25456,  25582,  25707,  25831,  25954,  26077,  26198,
  26318,  26437,  26556,  26673,  26789,  26905,  27019,  27132,
  27244,  27355,  27466,  27575,  27683,  27790,  27896,  28001,
  28105,  28208,  28309,  28410,  28510,  28608,  28706,  28802,
  28897,  28992,  29085,  29177,  29268,  29358,  29446,  29534,
  29621,  29706,  29790,  29873,  29955,  30036,  30116,  30195,
  30272,  30349,  30424,  30498,  30571,  30643,  30713,  30783,
  30851,  30918,  30984,  31049,  31113,  31175,  31236,  31297,
  31356,  31413,  31470,  31525,  31580,  31633,  31684,  31735,
  31785,  31833,  31880,  31926,  31970,  32014,  32056,  32097,
  32137,  32176,  32213,  32249,  32284,  32318,  32350,  32382,
  32412,  32441,  32468,  32495,  32520,  32544,  32567,  32588,
  32609,  32628,  32646,  32662,  32678,  32692,  32705,  32717,
  32727,  32736,  32744,  32751,  32757,  32761,  32764,  32766,
  32767,  32766,  32764,  32761,  32757,  32751,  32744,  32736,
  32727,  32717,  32705,  32692,  32678,  32662,  32646,  32628,
  32609,  32588,  32567,  32544,  32520,  32495,  32468,  32441,
  32412,  32382,  32350,  32318,  32284,  32249,  32213,  32176,
  32137,  32097,  32056,  32014,  31970,  31926,  31880,  31833,
  31785,  31735,  31684,  31633,  31580,  31525,  31470,  31413,
  31356,  31297,  31236,  31175,  31113,  31049,  30984,  30918,
  30851,  30783,  30713,  30643,  30571,  30498,  30424,  30349,
  30272,  30195,  30116,  30036,  29955,  29873,  29790,  29706,
  29621,  29534,  29446,  29358,  29268,  29177,  29085,  28992,
  28897,  28802,  28706,  28608,  28510,  28410,  28309,  28208,
  28105,  28001,  27896,  27790,  27683,  27575,  27466,  27355,
  27244,  27132,  27019,  26905,  26789,  26673,  26556,  26437,
  26318,  26198,  26077,  25954,  25831,  25707,  25582,  25456,
  25329,  25201,  25072,  24942,  24811,  24679,  24546,  24413,
  24278,  24143,  24006,  23869,  23731,  23592,  23452,  23311,
  23169,  23027,  22883,  22739,  22594,  22448,  22301,  22153,
  22004,  21855,  21705,  21554,  21402,  21249,  21096,  20942,
  20787,  20631,  20474,  20317,  20159,  20000,  19840,  19680,
  19519,  19357,  19194,  19031,  18867,  18702,  18537,  18371,
  18204,  18036,  17868,  17699,  17530,  17360,  17189,  17017,
  16845,  16672,  16499,  16325,  16150,  15975,  15799,  15623,
  15446,  15268,  15090,  14911,  14732,  14552,  14372,  14191,
  14009,  13827,  13645,  13462,  13278,  13094,  12909,  12724,
  12539,  12353,  12166,  11980,  11792,  11604,  11416,  11227,
  11038,  10849,  10659,  10469,  10278,  10087,   9895,   9703,
   9511,   9319,   9126,   8932,   8739,   8545,   8351,   8156,
   7961,   7766,   7571,   7375,   7179,   6982,   6786,   6589,
   6392,   6195,   5997,   5799,   5601,   5403,   5205,   5006,
   4807,   4608,   4409,   4210,   4011,   3811,   3611,   3411,
   3211,   3011,   2811,   2610,   2410,   2209,   2009,   1808,
   1607,   1406,   1206,   1005,    804,    603,    402,    201,
      0,   -201,   -402,   -603,   -804,  -1005,  -1206,  -1406,
  -1607,  -1808,  -2009,  -2209,  -2410,  -2610,  -2811,  -3011,
  -3211,  -3411,  -3611,  -3811,  -4011,  -4210,  -4409,  -4608,
  -4807,  -5006,  -5205,  -5403,  -5601,  -5799,  -5997,  -6195,
  -6392,  -6589,  -6786,  -6982,  -7179,  -7375,  -7571,  -7766,
  -7961,  -8156,  -8351,  -8545,  -8739,  -8932,  -9126,  -9319,
  -9511,  -9703,  -9895, -10087, -10278, -10469, -10659, -10849,
 -11038, -11227, -11416, -11604, -11792, -11980, -12166, -12353,
 -12539, -12724, -12909, -13094, -13278, -13462, -13645, -13827,
 -14009, -14191, -14372, -14552, -14732, -14911, -15090, -15268,
 -15446, -15623, -15799, -15975, -16150, -16325, -16499, -16672,
 -16845, -17017, -17189, -17360, -17530, -17699, -17868, -18036,
 -18204, -18371, -18537, -18702, -18867, -19031, -19194, -19357,
 -19519, -19680, -19840, -20000, -20159, -20317, -20474, -20631,
 -20787, -20942, -21096, -21249, -21402, -21554, -21705, -21855,
 -22004, -22153, -22301, -22448, -22594, -22739, -22883, -23027,
 -23169, -23311, -23452, -23592, -23731, -23869, -24006, -24143,
 -24278, -24413, -24546, -24679, -24811, -24942, -25072, -25201,
 -25329, -25456, -25582, -25707, -25831, -25954, -26077, -26198,
 -26318, -26437, -26556, -26673, -26789, -26905, -27019, -27132,
 -27244, -27355, -27466, -27575, -27683, -27790, -27896, -28001,
 -28105, -28208, -28309, -28410, -28510, -28608, -28706, -28802,
 -28897, -28992, -29085, -29177, -29268, -29358, -29446, -29534,
 -29621, -29706, -29790, -29873, -29955, -30036, -30116, -30195,
 -30272, -30349, -30424, -30498, -30571, -30643, -30713, -30783,
 -30851, -30918, -30984, -31049, -31113, -31175, -31236, -31297,
 -31356, -31413, -31470, -31525, -31580, -31633, -31684, -31735,
 -31785, -31833, -31880, -31926, -31970, -32014, -32056, -32097,
 -32137, -32176, -32213, -32249, -32284, -32318, -32350, -32382,
 -32412, -32441, -32468, -32495, -32520, -32544, -32567, -32588,
 -32609, -32628, -32646, -32662, -32678, -32692, -32705, -32717,
 -32727, -32736, -32744, -32751, -32757, -32761, -32764, -32766,
};

/*
  FIX_MPY() - fixed-point multiplication & scaling.
  Substitute inline assembly for hardware-specific
  optimization suited to a particluar DSP processor.
  Scaling ensures that result remains 16-bit.
*/
inline short FIX_MPY(short a, short b)
{
    /* shift right one less bit (i.e. 15-1) */
    int c = ((int)a * (int)b) >> 14;
    /* last bit shifted out = rounding-bit */
    b = c & 0x01;
    /* last shift + rounding bit */
    a = (c >> 1) + b;
    return a;
}

/*
  fix_fft() - perform forward/inverse fast Fourier transform.
  fr[n],fi[n] are real and imaginary arrays, both INPUT AND
  RESULT (in-place FFT), with 0 <= n < 2**m; set inverse to
  0 for forward transform (FFT), or 1 for iFFT.
*/
int fix_fft(short fr[], short fi[], short m, short inverse)
{
    int mr, nn, i, j, l, k, istep, n, scale, shift;
    short qr, qi, tr, ti, wr, wi;

    n = 1 << m;

    /* max FFT size = N_WAVE */
    if (n > N_WAVE)
        return -1;

    mr = 0;
    nn = n - 1;
    scale = 0;

    /* decimation in time - re-order data */
    for (m=1; m<=nn; ++m) {
        l = n;
        do {
            l >>= 1;
        } while (mr+l > nn);
        mr = (mr & (l-1)) + l;

        if (mr <= m)
            continue;
        tr = fr[m];
        fr[m] = fr[mr];
        fr[mr] = tr;
        ti = fi[m];
        fi[m] = fi[mr];
        fi[mr] = ti;
    }

    l = 1;
    k = LOG2_N_WAVE-1;
    while (l < n) {
        if (inverse) {
            /* variable scaling, depending upon data */
            shift = 0;
            for (i=0; i<n; ++i) {
                j = fr[i];
                if (j < 0)
                    j = -j;
                m = fi[i];
                if (m < 0)
                    m = -m;
                if (j > 16383 || m > 16383) {
                    shift = 1;
                    break;
                }
            }
            if (shift)
                ++scale;
        } else {
            /*
              fixed scaling, for proper normalization --
              there will be log2(n) passes, so this results
              in an overall factor of 1/n, distributed to
              maximize arithmetic accuracy.
            */
            shift = 1;
        }
        /*
          it may not be obvious, but the shift will be
          performed on each data point exactly once,
          during this pass.
        */
        istep = l << 1;
        for (m=0; m<l; ++m) {
            j = m << k;
            /* 0 <= j < N_WAVE/2 */
            wr =  Sinewave[j+N_WAVE/4];
            wi = -Sinewave[j];
            if (inverse)
                wi = -wi;
            if (shift) {
                wr >>= 1;
                wi >>= 1;
            }
            for (i=m; i<n; i+=istep) {
                j = i + l;
                tr = FIX_MPY(wr,fr[j]) - FIX_MPY(wi,fi[j]);
                ti = FIX_MPY(wr,fi[j]) + FIX_MPY(wi,fr[j]);
                qr = fr[i];
                qi = fi[i];
                if (shift) {
                    qr >>= 1;
                    qi >>= 1;
                }
                fr[j] = qr - tr;
                fi[j] = qi - ti;
                fr[i] = qr + tr;
                fi[i] = qi + ti;
            }
        }
        --k;
        l = istep;
    }
    return scale;
}

/*
  fix_fftr() - forward/inverse FFT on array of real numbers.
  Real FFT/iFFT using half-size complex FFT by distributing
  even/odd samples into real/imaginary arrays respectively.
  In order to save data space (i.e. to avoid two arrays, one
  for real, one for imaginary samples), we proceed in the
  following two steps: a) samples are rearranged in the real
  array so that all even samples are in places 0-(N/2-1) and
  all imaginary samples in places (N/2)-(N-1), and b) fix_fft
  is called with fr and fi pointing to index 0 and index N/2
  respectively in the original array. The above guarantees
  that fix_fft "sees" consecutive real samples as alternating
  real and imaginary samples in the complex array.
*/
int fix_fftr(short f[], int m, int inverse)
{
    int i, N = 1<<(m-1), scale = 0;
    short tt, *fr=f, *fi=&f[N];

    if (inverse)
        scale = fix_fft(fi, fr, m-1, inverse);
    for (i=1; i<N; i+=2) {
        tt = f[N+i-1];
        f[N+i-1] = f[i];
        f[i] = tt;
    }
    if (! inverse)
        scale = fix_fft(fi, fr, m-1, inverse);
    return scale;
}
#endif
