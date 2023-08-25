#include <cmath>
#include <iostream>

#define PI_CON 3.14159f

class Oscillator {
    private:
        float _freq;
        float _amp;
        float _phase;
        float _samp_rate;
        float _increment = 0.0f;
        float _cur_val = 0.0f;

    public:
        Oscillator() : 
        _freq(0.0f), _amp(0.0f), _phase(0.0f), _samp_rate(0.0f){}

        Oscillator(float freq, float amp, float phase, float samp_rate): 
        _freq(freq), _amp(amp), _phase(phase), _samp_rate(samp_rate){}

        float get_cur_value() { return _cur_val; }
        void set_cur_val(float val) { _cur_val = val; }
        virtual float get_next_value() = 0;

        float get_freq() { return _freq; }
        void set_freq(float freq) { _freq = freq; }

        float get_amp() { return _amp; }
        void set_amp(float amp) { _amp = amp; }

        float get_phase() { return _phase; }
        void set_phase(float phase) { _phase = phase; }

        float get_inc() { return _increment; }
        void set_inc(float increment) { _increment = increment; }

        float get_samp_rate() { return _samp_rate; }

};

class SineOsc : Oscillator {
    private:
        float _step;

    public:
        SineOsc() {
            _step = 0.0f;
        }

        SineOsc(float freq, float amp, float phase, float samp_rate) : Oscillator(freq, amp, phase, samp_rate) {
            calc_step();
        }

        void set_freq(float freq) {
            Oscillator::set_freq(freq);
            this->calc_step();
        }

        void set_amp(float amp) { Oscillator::set_amp(amp); }
        float get_amp() { return Oscillator::get_amp(); }

        void calc_step() {
            _step = (2 * PI_CON * this->get_freq()) / this->get_samp_rate();
        }

        float get_next_value() {
            if(this->get_inc() - this->get_samp_rate() > 0.001f)
                this->set_inc(0.0f);

            float next_value = sin(this->get_inc() + this->get_phase());

            this->set_inc(this->get_inc() + _step);
            set_cur_val(this->get_amp()*next_value);

            return this->get_amp()*next_value;
        }
};

class SawOsc : Oscillator {
    private:
        float _period;

    public:
        SawOsc() {
            _period = 0.0f;
        }

        SawOsc(float freq, float amp, float phase, float samp_rate) : Oscillator(freq, amp, phase, samp_rate) {
            calc_period();
        }

        void set_freq(float freq) {
            Oscillator::set_freq(freq);
            this->calc_period();
        }

        void set_amp(float amp) { Oscillator::set_amp(amp); }
        float get_amp() { return Oscillator::get_amp(); }


        void calc_period() {
            this->_period = this->get_samp_rate()/this->get_freq();
        }

        float get_next_value()
        {
            float div = (this->get_inc() + this->get_phase())/this->_period;
            float next_val = 2 * (div - floor(0.5 + div)); 
            
            this->set_inc(this->get_inc() + 1);

            this->set_cur_val(next_val * this->get_amp());

            return (next_val * this->get_amp());
        }

};

class SqOsc : Oscillator {
    private:
        float _step;
        float _threshold;


    public:
        SqOsc() {
            _step = 0.0f;
            _threshold = 0.0f;
        }

        SqOsc(float freq, float amp, float phase, float samp_rate, float threshold) : Oscillator(freq, amp, phase, samp_rate) {
            _threshold = threshold;

            calc_step();
        }

        void set_freq(float freq) {
            Oscillator::set_freq(freq);
            this->calc_step();
        }

        void set_amp(float amp) { Oscillator::set_amp(amp); }
        float get_amp() { return Oscillator::get_amp(); }


        void calc_step() {
            _step = (2 * PI_CON * this->get_freq()) / this->get_samp_rate();
        }

        float get_next_value() {
            if(this->get_inc() - this->get_samp_rate() > 0.001f)
                this->set_inc(0.0f);

            float next_value = (sin(this->get_inc() + this->get_phase()) - _threshold > 0.001f) ? 0 : 1;

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

        int16_t _upper_bound;
        int16_t _lower_bound;

    public:
        WaveAdder(float freq, float amp, float phase, float samp_rate, float suplimit, float lowlimit) {
            SineOsc sinaux(freq, amp, phase, samp_rate);
            SqOsc sqaux(freq, amp, phase, samp_rate, 0.0f);
            SawOsc sawaux(freq, amp, phase, samp_rate);

            _sine = sinaux;
            _square = sqaux;
            _sawtooth = sawaux;

            _upper_bound = suplimit;
            _lower_bound = lowlimit;
        }

        void change_freq(float freq) {
            _sine.set_freq(freq);
            _square.set_freq(freq);
            _sawtooth.set_freq(freq);
        }

        void change_sin_amp(float amp) { _sine.set_amp(amp); }
        void change_squ_amp(float amp) { _square.set_amp(amp); }
        void change_saw_amp(float amp) { _sawtooth.set_amp(amp); }

        float get_next_value() {
            float val = _sine.get_amp()*_sine.get_next_value() + 
                        _square.get_amp()*_square.get_next_value() + 
                        _sawtooth.get_amp()*_sawtooth.get_next_value();

            if(val > _upper_bound) val = _upper_bound;
            if(val < _lower_bound) val = _lower_bound;

            return val;
        }
        
};

int main() {
    WaveAdder wa(440.0f, 1.0f, 0.0f, 44100.0f, 1.0f, -1.0f);
    wa.change_squ_amp(1.0f);
    wa.change_saw_amp(1.0f);


    for(auto i = 0; i < 1024; i++) {
        std::cout<<wa.get_next_value()<<std::endl;
    }


    return 0;
}