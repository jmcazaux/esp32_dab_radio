

class AudioSource {
   public:
    const char* name;
    const bool needsRadio = false;
    const bool needsBluetooth = false;
    const bool needsLowCpuFrequency = false;

    virtual void tuneUp(){};
    virtual void tuneDown(){};
    virtual void tunePressed(){};
    virtual void tuneLongPressed(){};
    virtual void tuneDoublePressed(){};

    AudioSource(const char* _name) : name(_name) {};
};
