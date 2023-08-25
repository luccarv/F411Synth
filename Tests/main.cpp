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
        SineOsc sine;
        SqOsc square;
        SawOsc sawtooth;

        int16_t upper_bound;
        int16_t lower_bound;

    public:
        WaveAdder(float freq, float amp, float phase, float samp_rate, int16_t *limits) {
            SineOsc sinaux(freq, amp, phase, samp_rate);
            SqOsc sqaux(freq, amp, phase, samp_rate, 0.0f);
            SawOsc sawaux(freq, amp, phase, samp_rate);

            sine = sinaux;
            square = sqaux;
            sawtooth = sawaux;

            upper_bound = limits[0];
            lower_bound = limits[1];
        }

        void change_freq(float freq) {
            sine.set_freq(freq);
            square.set_freq(freq);
            sawtooth.set_freq(freq);
        }

        void change_sin_amp(float amp) { sine.set_amp(amp); }
        void change_squ_amp(float amp) { square.set_amp(amp); }
        void change_saw_amp(float amp) { sawtooth.set_amp(amp); }

        
};

int main() {
    SqOsc sq(440.0f, 0.5f, 0.0f, 44100.0f, 0.0f);
    SineOsc si(440.0f, 0.3f, 0.0f, 44100.0f);
    SawOsc sa(440.0f, 0.2f, 0.0f, 44100.0f);
    
    for(auto i = 0; i < 1024; i++) {
        //sq.get_next_value();

        if(i == 512)
            sq.set_freq(220.0f);

        std::cout<<sq.get_next_value()<<std::endl;
    }
    std::cout<<sq.get_next_value();


    return 0;
}