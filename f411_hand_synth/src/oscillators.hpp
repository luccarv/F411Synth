#include <cmath>
#include <iostream>
#include <arm_math.h>    
#include <math.h>


#define PI_CON 3.14159f

class Oscillator {
    private:
        float_t _freq;
        float_t _amp;
        float_t _phase;
        float_t _samp_rate;
        float_t _increment = 0.0f;
        float_t _cur_val = 0.0f;

    public:
        Oscillator() : 
        _freq(0.0f), _amp(0.0f), _phase(0.0f), _samp_rate(0.0f){}

        Oscillator(float_t freq, float_t amp, float_t phase, float_t samp_rate): 
        _freq(freq), _amp(amp), _phase(phase), _samp_rate(samp_rate){}

        float_t get_cur_value() { return _cur_val; }
        void set_cur_val(float_t val) { _cur_val = val; }
        virtual float_t get_next_value() = 0;

        float_t get_freq() { return _freq; }
        void set_freq(float_t freq) { _freq = freq; }

        float_t get_amp() { return _amp; }
        void set_amp(float_t amp) { _amp = amp; }

        float_t get_phase() { return _phase; }
        void set_phase(float_t phase) { _phase = phase; }

        float_t get_inc() { return _increment; }
        void set_inc(float_t increment) { _increment = increment; }

        float_t get_samp_rate() { return _samp_rate; }

};

class SineOsc : Oscillator {
    private:
        float_t _step;

    public:
        SineOsc() {
            _step = 0.0f;
        }

        SineOsc(float_t freq, float_t amp, float_t phase, float_t samp_rate) : Oscillator(freq, amp, phase, samp_rate) {
            calc_step();
        }

        void set_freq(float_t freq) {
            Oscillator::set_freq(freq);
            this->calc_step();
        }

        void set_amp(float_t amp) { Oscillator::set_amp(amp); }
        float_t get_amp() { return Oscillator::get_amp(); }

        void calc_step() {
            _step = (2 * PI_CON * this->get_freq()) / this->get_samp_rate();
        }

        float_t get_next_value() {
            if(this->get_inc() - this->get_samp_rate() > 0.001f)
                this->set_inc(0.0f);

            float_t next_value = arm_sin_f32(this->get_inc() + this->get_phase());

            this->set_inc(this->get_inc() + _step);
            set_cur_val(this->get_amp()*next_value);

            return this->get_amp()*next_value;
        }
};

class SawOsc : Oscillator {
    private:
        float_t _period;

    public:
        SawOsc() {
            _period = 0.0f;
        }

        SawOsc(float_t freq, float_t amp, float_t phase, float_t samp_rate) : Oscillator(freq, amp, phase, samp_rate) {
            calc_period();
        }

        void set_freq(float_t freq) {
            Oscillator::set_freq(freq);
            this->calc_period();
        }

        void set_amp(float_t amp) { Oscillator::set_amp(amp); }
        float_t get_amp() { return Oscillator::get_amp(); }


        void calc_period() {
            this->_period = this->get_samp_rate()/this->get_freq();
        }

        float_t get_next_value()
        {
            float_t div = (this->get_inc() + this->get_phase())/this->_period;
            float_t next_val = 2 * (div - floor(0.5 + div)); 
            
            this->set_inc(this->get_inc() + 1);

            this->set_cur_val(next_val * this->get_amp());

            return (next_val * this->get_amp());
        }

};

class SqOsc : Oscillator {
    private:
        float_t _step;
        float_t _threshold;


    public:
        SqOsc() {
            _step = 0.0f;
            _threshold = 0.0f;
        }

        SqOsc(float_t freq, float_t amp, float_t phase, float_t samp_rate, float_t threshold) : Oscillator(freq, amp, phase, samp_rate) {
            _threshold = threshold;

            calc_step();
        }

        void set_freq(float_t freq) {
            Oscillator::set_freq(freq);
            this->calc_step();
        }

        void set_amp(float_t amp) { Oscillator::set_amp(amp); }
        float_t get_amp() { return Oscillator::get_amp(); }


        void calc_step() {
            _step = (2 * PI_CON * this->get_freq()) / this->get_samp_rate();
        }

        float_t get_next_value() {
            if(this->get_inc() - this->get_samp_rate() > 0.001f)
                this->set_inc(0.0f);

            float_t next_value = (arm_sin_f32(this->get_inc() + this->get_phase()) - _threshold > 0.001f) ? 0 : 1;

            this->set_inc(this->get_inc() + _step);
            set_cur_val(this->get_amp()*next_value);

            return this->get_amp()*next_value;
        }
};

class WaveAdder{
    private:
        SineOsc _sine;
        SqOsc _square;
        SawOsc _sawtooth;

        float_t _upper_bound;
        float_t _lower_bound;

    public:
        WaveAdder(float_t freq, float_t amp, float_t phase, float_t samp_rate, float_t suplimit, float_t lowlimit) {
            SineOsc sinaux(freq, amp, phase, samp_rate);
            SqOsc sqaux(freq, amp, phase, samp_rate, 0.0f);
            SawOsc sawaux(freq, amp, phase, samp_rate);

            _sine = sinaux;
            _square = sqaux;
            _sawtooth = sawaux;

            _upper_bound = suplimit;
            _lower_bound = lowlimit;
        }

        void change_freq(float_t freq) {
            _sine.set_freq(freq);
            _square.set_freq(freq);
            _sawtooth.set_freq(freq);
        }

        void change_sin_amp(float_t amp) { _sine.set_amp(amp); }
        void change_squ_amp(float_t amp) { _square.set_amp(amp); }
        void change_saw_amp(float_t amp) { _sawtooth.set_amp(amp); }

        float_t get_sin_amp() { return _sine.get_amp(); }
        float_t get_squ_amp() { return _square.get_amp(); }
        float_t get_saw_amp() { return _sawtooth.get_amp(); }

        float_t get_next_value() {
            float_t val = _sine.get_next_value() + 
                        _square.get_next_value() + 
                        _sawtooth.get_next_value();

            if(val > _upper_bound) val = _upper_bound;
            if(val < _lower_bound) val = _lower_bound;

            return val;
        }
        
};