

class AudioSource {
  public:
   const char* name;
   const bool needsRadio;
   const bool needsBluetooth;
   const bool needsLowCpuFrequency;

   virtual void tuneUp(bool fast);
   virtual void tuneDown(bool fast);
};
