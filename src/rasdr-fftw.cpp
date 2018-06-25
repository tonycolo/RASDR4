#include "FFTFrameGenerator.h"
#include "ConnectionRegistry.h"
#include "ConnectionHandle.h"
#include "LMS7002M_parameters.h"
#include "IConnection.h"
#include "LimeSuite.h"
#include "LMS7002M.h"
#include <iostream>
#include <stdint-gcc.h>
#include <sstream>

#include "FFTFileWriter.h"
#include "AudioDemodulate.h"
#include "Properties.h"

using namespace std;


//Device structure, should be initialize to NULL
lms_device_t* device = NULL;

StreamCircularBuffer* streamCircularBuffera;
StreamCircularBuffer* framesCircularBuffera;
StreamCircularBuffer* streamCircularBufferb;
StreamCircularBuffer* framesCircularBufferb;
StreamCircularBuffer::StreamingParameters *streama;
StreamCircularBuffer::StreamingParameters *streamb;

int error()
{
    //print head error message
    cout << "ERROR:" << LMS_GetLastErrorMessage();
    if (device != NULL)
        LMS_Close(device);
    exit(-1);
}

/**
 * Simple convenience function to compare single char string
 * specifying band.
 * @param band {L,H,W}
 * @return LMS_PATH_X enumerated value
 */
size_t mapStringToBandEnum(std::string band) {
    if (band.compare("W")==0) {
        std::cout << "LMS_PATH_LNAW Set" << "\n";
        return LMS_PATH_LNAW;
    }
    if (band.compare("L")==0) {
        std::cout << "LMS_PATH_LNAL Set" << "\n";
        return LMS_PATH_LNAL;
    }
    if (band.compare("H")==0) {
        std::cout << "LMS_PATH_LNAH Set" << "\n";
        return LMS_PATH_LNAH;
    }
    return -999; //unknown
}

void setStreamA(StreamCircularBuffer::StreamingParameters *stream) {
    streama = stream;
}

void setStreamB(StreamCircularBuffer::StreamingParameters *stream) {
    streamb = stream;
}
/**
 * Primary function for setting device and continually reading I+Q samples
 * and pushing those into the circular buffers for exploitation.
 * @param streama streaming channel 1 parameters
 * @param streamb streaming channel 2 parameters
 */
void process(int loops) {
    //Find devices
    double center_frequency = streama->center_frequency_hz;//407.8e6;
    int n;
    lms_info_str_t list[8]; //should be large enough to hold all detected devices
    if ((n = LMS_GetDeviceList(list)) < 0) //NULL can be passed to only get number of devices
        error();

    cout << "Devices found: " << n << endl; //print number of devices
    if (n < 1)
        return;

    //open the first device
    if (LMS_Open(&device, list[0], NULL))
        error();

    //lime::LMS7002M lms;
    //auto cachedHandles = lime::ConnectionRegistry::findConnections();
    //if (cachedHandles.size() > 0) {
        //lms.SetConnection(lime::ConnectionRegistry::makeConnection(cachedHandles.at(0)));
    //}
    //std::vector<ConnectionHandle> connections = ConnectionRegistry::findConnections();
    //Initialize device with default configuration
    //Do not use if you want to keep existing configuration

    //if (LMS_LoadConfig(device, "rx-408-no-spurs2.ini")!=0)
    //    error();// to load config from INI

    LMS_Reset(device);
    //if no config file:
    if (LMS_Init(device) != 0)
       error;

    size_t rfe_path_1; //radio front end path aka "antenna" 1
    size_t rfe_path_2; //radio front end path aka "antenna" 2

    rfe_path_1 = mapStringToBandEnum(streama->band);
    rfe_path_2 = mapStringToBandEnum(streamb->band);

    if (LMS_SetAntenna(device, LMS_CH_RX, 0, rfe_path_1)) {
        error();
    }

    if (LMS_SetAntenna(device, LMS_CH_RX, 1, rfe_path_2)) {
        error();
    }

    if (LMS_EnableChannel(device, LMS_CH_RX, 0, true) != 0)
        error();
    if (LMS_EnableChannel(device, LMS_CH_RX, 1, true) != 0)
        error();


    //LMS_Set
    //Enable RX channel 1 (RX2)
    //Channels are numbered starting at 0

    //Set center frequency to 408 MHz
    if (LMS_SetLOFrequency(device, LMS_CH_RX, 0, center_frequency) != 0)
        error();
    if (LMS_SetLOFrequency(device, LMS_CH_RX, 1, center_frequency) != 0)
        error();

    LMS_SetLPF(device,false,0,true);
    LMS_SetLPFBW(device,false,0,streama->lowpass_bandwidth_hz);

    LMS_SetLPF(device,false,1,true);
    LMS_SetLPFBW(device,false,1,streama->lowpass_bandwidth_hz);
    float_type hz;
    float_type rf_hz;

    //if (LMS_SetLOFrequency(device, LMS_CH_RX, 1, center_frequency) != 0)
    //    error();
    //Set sample rate to 8 MHz, ask to use 32x oversampling in RF
    //This set sampling rate for all channels
    double sample_rate = streama->sampling_rate_sps;
    int decimation_ratio = streama->decimation;
    if (LMS_SetSampleRate(device, sample_rate, decimation_ratio) != 0)
        error();

    LMS_GetSampleRate(device,false,0,&hz,&rf_hz);
    printf("Effective sample rate RF sample rate %.1f %.1f\n",hz,rf_hz);

    streama->effective_sample_rate = rf_hz;
    streamb->effective_sample_rate = rf_hz;

    unsigned int gain;
    unsigned int gain2;
    //LMS_SetGaindB(device,false,0,1);
    LMS_GetGaindB(device,false,0,&gain);

    /* setting LNA 13 TIA 1 PGA 1= total gain of 29.  The LNA setting is non-linear with respect to dB gain */
    uint16_t lna = 15; //15 = Gmax, 14 = Gmax -1, 13 = Gmax -2, 2 = GMAX-27.  LNA max setting = 15 (31 dB), min = 1 (1 db)
    uint16_t tia = 1; //3= Gmax, 2= Gmax-3 1 = Gmax-12                      TIA max settug  = 3  (12 dB), min = 1 (0 db)
    uint16_t pga = 1; //db = PGA -1,  1 = 0db, 2 = 1db, 31 = 30db           PGA max setting = 31 (30 dB), min = 1 (0 dB)

    if (LMS_EnableChannel(device, LMS_CH_RX, 0, true) != 0)
        error();

    //Channel A/1
    int result = LMS_WriteParam(device,LMS7param(G_LNA_RFE),lna);
    result = LMS_WriteParam(device,LMS7param(G_TIA_RFE),tia);
    result = LMS_WriteParam(device,LMS7param(G_PGA_RBB),pga);

    result = LMS_WriteParam(device,LMS7param(AGC_MODE_RXTSP),1);
    result = LMS_WriteParam(device,LMS7param(CMIX_BYP_RXTSP),1);

    //Channel B/2
    if (LMS_EnableChannel(device, LMS_CH_RX, 1, true) != 0)
        error();
    result = LMS_WriteParam(device,LMS7param(G_LNA_RFE),lna);
    result = LMS_WriteParam(device,LMS7param(G_TIA_RFE),tia);
    result = LMS_WriteParam(device,LMS7param(G_PGA_RBB),pga);

    result = LMS_WriteParam(device,LMS7param(AGC_MODE_RXTSP),1);
    result = LMS_WriteParam(device,LMS7param(CMIX_BYP_RXTSP),1);
    result = LMS_WriteParam(device,LMS7param(DCCORR_AVG_RXTSP),7);  //2^14+n samples for DC averaging
    result = LMS_WriteParam(device,LMS7param(DC_BYP_RXTSP),0);

    //lime::LMS7002M lms;
    /*lms.SetRFELNA_dB(27);
    lms.SetRBBPGA_dB(-12);
    lms.SetRFETIA_dB(0);
    lms.UploadAll();
    */
    //float pga_dba = lms.GetRBBPGA_dB();

    //printf("LMS7002M GetRBBPGA_dB %.1f\n",pga_dba);
    //printf("LMS7002M GetRFELNA_db %.1f\n",lms.GetRFELNA_dB());


    //Enable test signal generation
    //To receive data from RF, remove this line or change signal to LMS_TESTSIG_NONE
    //if (LMS_SetTestSignal(device, LMS_CH_RX, 0, LMS_TESTSIG_NCODIV8, 0, 0) != 0)
    //   error();
    //if (LMS_SetTestSignal(device, LMS_CH_RX, 0, LMS_TESTSIG_NCODIV8, 0, 0) != 0) {
    //    error();
    //}
        //    error();
    //printf("gain 1 %d (dB)\n",gain);

    //LMS_Calibrate(device,false,0,streama->lowpass_bandwidth_hz,0);
     //       int total_gain = 30;

   // LMS_SetGaindB(device,LMS_CH_RX,0,total_gain);
    LMS_GetGaindB(device,false,0,&gain);
    //LMS_Calibrate(device,false,1,bandwidth,0);

    LMS_GetGaindB(device,false,1,&gain2);
    printf("gain after setting LNA PGA TIA RX1 RX2 %d %d (dB)\n",gain,gain2);
    //LMS_Calibrate(device,false,1,bandwidth,0);
    //Streaming Setup

    //Initialize stream
    lms_stream_t streamAId;                        //stream structure
    streamAId.channel = 0;                         //channel number
    streamAId.fifoSize = 1024 * 1024;              //fifo size in samples3
    streamAId.throughputVsLatency = 1.0;           //optimize for max throughput
    streamAId.isTx = false;                        //RX channel
    streamAId.dataFmt = lms_stream_t::LMS_FMT_F32; //32 bit floats

    lms_stream_t streamBId;                        //stream structure
    streamBId.channel = 1;                         //channel number
    streamBId.fifoSize = 1024 * 1024;              //fifo size in samples
    streamBId.throughputVsLatency = 1.0;           //optimize for max throughput
    streamBId.isTx = false;                        //RX channel
    streamBId.dataFmt = lms_stream_t::LMS_FMT_F32; //32-bit floats

    if (LMS_SetupStream(device, &streamAId) != 0)
        error();
    if (LMS_SetupStream(device, &streamBId) != 0)
        error();

    //if (LMS_SetupStream(device, &streamBId) != 0)
    //    error();
    //Initialize data buffers

    //Start streaming
    LMS_StartStream(&streamAId);
    LMS_StartStream(&streamBId);
    //auto t1 = chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point now = chrono::high_resolution_clock::now();

    streama->first_time = now;
    streamb->first_time = now;

    //Streaming
    const int sampleCnt = streama->sample_size_per_block; //complex samples per buffer
    int samples = 0;
    int frame = 0;
    lms_stream_meta_t metaa;
    lms_stream_meta_t metab;
    //int16_t buffera[sampleCnt*2];
    //int16_t bufferb[sampleCnt*2];

    float buffera[sampleCnt*2];
    float bufferb[sampleCnt*2];
    int loop = 0;
    while (loop < loops) {
        //Receive samples, timeout at 100msec
        //I and Q samples are interleaved in buffer: IQIQIQ...
        int samplesReada = LMS_RecvStream(&streamAId, &buffera, sampleCnt, &metaa, 100);
        int samplesReadb = LMS_RecvStream(&streamBId, &bufferb, sampleCnt, &metab, 100);
        samples = samples + samplesReada;
        streamCircularBuffera->pushBlock(buffera, samplesReada,metaa.timestamp);  //notify for the object that the data is ready
        streamCircularBufferb->pushBlock(bufferb, samplesReadb,metab.timestamp);
        loop++;
    }
    //Stop streaming
    LMS_StopStream(&streamAId); //stream is stopped but can be started again with LMS_StartStream()
    LMS_DestroyStream(device, &streamAId); //stream is deallocated and can no longer be used
    //lime::IConnection* connection = lms.GetConnection();
    //lime::ConnectionRegistry::freeConnection(connection);
    //Close device
    LMS_Close(device);
    printf("Received %d samples\n", samples);
    return;
}

/**
 * Create the buffers for the blocks (each collection of samples) and FFT frames.
 * @param stream streaming parameters, including sample size per block
 * @param buffer_size how many blocks to allocate
 */
void createBuffers(StreamCircularBuffer::StreamingParameters streama, StreamCircularBuffer::StreamingParameters streamb, int buffer_size) {
    streamCircularBuffera = new StreamCircularBuffer(streama.sample_size_per_block,buffer_size,2); //2 dimensions for I+Q
    framesCircularBuffera = new StreamCircularBuffer(streama.sample_size_per_block,buffer_size,1); //1 dimension for power
    streamCircularBufferb = new StreamCircularBuffer(streamb.sample_size_per_block,buffer_size,2);
    framesCircularBufferb = new StreamCircularBuffer(streamb.sample_size_per_block,buffer_size,1);
}

StreamCircularBuffer* getStreamCircularBuffera() {
    return streamCircularBuffera;
}

StreamCircularBuffer* getFramesCircularBuffera() {
    return framesCircularBuffera;
}

StreamCircularBuffer* getStreamCircularBufferb() {
    return streamCircularBufferb;
}

StreamCircularBuffer* getFramesCircularBufferb() {
    return framesCircularBufferb;
}

/**
 * FFTFrameGenerator will calculate additional stream parameters and store in stream instance
 * for use by FFTFileWriter and others.
 * Due to thread safety design of FFTW planner, must invoke that sequentially/in one thread for each FFTFrameGenerator
 *
 * @param stream device streaming parameters
 */
FFTFrameGenerator initFFTa(StreamCircularBuffer::StreamingParameters *streama) {
    StreamCircularBuffer *streamCircularBuffer1a = getStreamCircularBuffera();
    StreamCircularBuffer *framesCircularBuffer1a = getFramesCircularBuffera();
    FFTFrameGenerator myffta;
    myffta.setBuffers(streamCircularBuffer1a,framesCircularBuffer1a);
    myffta.init(streama);
    return myffta;
}

FFTFrameGenerator initFFTb(StreamCircularBuffer::StreamingParameters* streamb) {
    StreamCircularBuffer *streamCircularBuffer1b = getStreamCircularBufferb();
    StreamCircularBuffer *framesCircularBuffer1b = getFramesCircularBufferb();
    FFTFrameGenerator myfftb; //(streamCircularBuffer1b,framesCircularBuffer1b);
    myfftb.setBuffers(streamCircularBuffer1b,framesCircularBuffer1b);
    myfftb.init(streamb);
    //myfftb.setAudioStream(audioStream);
    return myfftb;
}

void startFFT(FFTFrameGenerator myfft) {
    myfft.start(1);
}

/**

 * @param stream
 */
void startFFTWriter(StreamCircularBuffer::StreamingParameters* streama, StreamCircularBuffer::StreamingParameters* streamb) {
    StreamCircularBuffer *framesCircularBuffer1 = getFramesCircularBuffera();
    StreamCircularBuffer *framesCircularBuffer2 = getFramesCircularBufferb();
    FFTFileWriter fftwriter;
    fftwriter.init("fft",framesCircularBuffer1,framesCircularBuffer2,streama,streamb);
    fftwriter.start();
}

void startGraph(StreamCircularBuffer::StreamingParameters* streama) {
    StreamCircularBuffer *framesCircularBuffer1 = getFramesCircularBuffera();
    StreamCircularBuffer *framesCircularBuffer2 = getFramesCircularBufferb();
    //PowerSpectrumDisplay power;//(framesCircularBuffer1);
    //power.init(framesCircularBuffer1,framesCircularBuffer2,streama);
    //power.start();
}

void startAudio(StreamCircularBuffer* buffer, int samples ) {
    AudioDemodulate *am = new AudioDemodulate();
    am->init(buffer, samples);
}

/**
 * Given a desired dB for LNA, compute the setting value for register.
 * setting LNA 13 TIA 1 PGA 1= total gain of 29.  The LNA setting is non-linear with respect to dB gain
 * uint16_t lna = 15; //15 = Gmax, 14 = Gmax -1, 13 = Gmax -2, 2 = GMAX-27.LNA max setting = 15 (31 dB), min = 1 (1 db)
 * uint16_t tia = 1; //3= Gmax, 2= Gmax-3 1 = Gmax-12                      TIA max setting  = 3  (12 dB), min = 1 (0 db)
 * uint16_t pga = 1; //db = PGA -1,  1 = 0db, 2 = 1db, 31 = 30db           PGA max setting = 31 (30 dB), min = 1 (0 dB)
 **/
uint16_t computeLNASetting(int dB) {
    if (dB <= 0) {
        return 1;
    }
    if (dB > 31) {
        return 15;
    }
    switch (dB){
        case 1: return 1;
        case 2: return 2;
        case 25 : return 9;
        case 26 : return 10;
        case 27 : return 11;
        case 28 : return 12;
        case 29 : return 13;
        case 30 : return 14;
        case 31 : return 15;
    }
    return 15; //default to max
}


/**
 * Given a comma-separated list, convert each element to a double
 * and store in vector.
 * @param list comma-separated list of numbers
 * @return vector of doubles
 */
vector<double> parseDoubles(std::string list) {
    vector<double> values;
    istringstream f(list);
    string s;
    while (std::getline(f, s, ',')) {
        values.push_back(std::stod(s));
    }
    return values;
}

vector<int> parseInts(std::string list) {
    vector<int> values;
    istringstream f(list);
    string s;
    while (std::getline(f, s, ',')) {
        values.push_back(std::stoi(s));
    }
    return values;
}

vector<std::string> parseString(std::string list) {
    vector<std::string> values;
    istringstream f(list);
    string s;
    while (std::getline(f, s, ',')) {
        values.push_back(s);
    }
    return values;
}

Properties::rasdr4 loadProperties() {
    Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> properties;
    properties = new Poco::Util::PropertyFileConfiguration("parameters.txt");
    Properties::rasdr4 rasdr4;
    if (properties->hasProperty("center-frequencies")) {
        rasdr4.center_frequencies = parseDoubles(properties->getString("center-frequencies"));
        rasdr4.center_frequency_hz = rasdr4.center_frequencies.at(0);
        printf("center-frequency %.4f\n", rasdr4.center_frequency_hz / 1e6);
    }
    if (properties->hasProperty("adc-clock-rates")) {
        vector<double> adc;
        adc = parseDoubles(properties->getString("adc-clock-rates"));
        rasdr4.cgen_freq_hz= adc.at(0);
        printf("adc clock rate (cg gen) %.1f\n", rasdr4.lowpass_bandwidth_hz);
    }
    if (properties->hasProperty("stream-samples")) {
        vector<int> stream_samples;
        stream_samples = parseInts(properties->getString("stream-samples"));
        rasdr4.sample_size_per_block = stream_samples.at(0);
        printf("IQ stream samples per block %d\n", rasdr4.sample_size_per_block);
    }
    if (properties->hasProperty("decimation-ratio")) {
        vector<int> decimation_ratios;
        decimation_ratios = parseInts(properties->getString("decimation-ratio"));
        rasdr4.decimation = decimation_ratios.at(0);
        printf("decimation ratio %d\n", rasdr4.decimation);
    }
    if (properties->hasProperty("fft-frame-size")) {
        vector<int> frames;
        frames = parseInts(properties->getString("fft-frame-size"));
        rasdr4.blocks_per_frame = frames.at(0);
        printf("fft frame size %d\n", rasdr4.blocks_per_frame);
    }
    if (properties->hasProperty("lowpass-bandwidth")) {
        vector<double> lpbw;
        lpbw = parseDoubles(properties->getString("lowpass-bandwidth"));
        rasdr4.lowpass_bandwidth_hz = lpbw.at(0);
        printf("lowpass bandwidth %.1f\n", rasdr4.lowpass_bandwidth_hz);
    }

    if (properties->hasProperty("network-port")) {
        vector<std::string> band;
        band = parseString(properties->getString("network-port"));
        rasdr4.band = band.at(0);
        printf("network port %s\n", rasdr4.band.c_str());
    }

    if (properties->hasProperty("effective-sample-rate")) {
        vector<double> efsr;
        efsr = parseDoubles(properties->getString("effective-sample-rate"));
        rasdr4.sampling_rate_sps = efsr.at(0);
        printf("effective sample rate %.1f\n", rasdr4.sampling_rate_sps);
    }
    if (properties->hasProperty("lna")) {
        vector<int> lna;
        lna = parseInts(properties->getString("lna"));
        rasdr4.lna_gain= lna.at(0);
        printf("LNA gain %d (dB)\n", rasdr4.lna_gain);
    }
    return rasdr4;
}


int main(int argc, char** argv) {
    Properties::rasdr4 rasdr4 = loadProperties();
    
    /* device streaming parameters */
    StreamCircularBuffer::StreamingParameters stream_a;
    stream_a.channel = "1";
    stream_a.band = rasdr4.band;
    stream_a.sample_size_per_block = rasdr4.sample_size_per_block;
    stream_a.blocks_per_frame = rasdr4.blocks_per_frame;
    stream_a.sampling_rate_sps = rasdr4.sampling_rate_sps;
    stream_a.center_frequency_hz = rasdr4.center_frequency_hz;
    stream_a.decimation = rasdr4.decimation;
    stream_a.lowpass_bandwidth_hz = rasdr4.lowpass_bandwidth_hz;

    StreamCircularBuffer::StreamingParameters stream_b;
    stream_b.channel = "2";
    stream_b.band = rasdr4.band;
    stream_b.sample_size_per_block = rasdr4.sample_size_per_block;
    stream_b.blocks_per_frame = rasdr4.blocks_per_frame;
    stream_b.sampling_rate_sps = rasdr4.sampling_rate_sps;
    stream_b.center_frequency_hz = rasdr4.center_frequency_hz;
    stream_b.decimation = rasdr4.decimation;
    stream_b.lowpass_bandwidth_hz = rasdr4.lowpass_bandwidth_hz;

    createBuffers(stream_a, stream_b,2048); //2048 is size of circular buffer in blocks

    if (fftw_init_threads() ==0)  //returns non-zero if successful
       printf("fftw_init_threads error!\n");
    fftw_plan_with_nthreads(4);

    /** MUST create FFTFrameGenerators sequentially because of FFTW Plan creation thread access **/
    FFTFrameGenerator ffta = initFFTa(&stream_a);
    FFTFrameGenerator fftb = initFFTb(&stream_b);

    auto audioBuffer = new StreamCircularBuffer(stream_a.sample_size_per_block, 1024, 1);
    //std::cout << audioBuffer->currentIndex() << " audioBuffer uuid " << audioBuffer->getUUID() << "\n";
    ffta.setAudioStream(audioBuffer);
    startAudio(audioBuffer, stream_a.sample_size_per_block);

    setStreamA(&stream_a);
    setStreamB(&stream_b);

    std::thread thread_ffta(startFFT,ffta);
    std::thread thread_fftb(startFFT,fftb);
    std::thread thread_stream(process,99999999);
    std::thread thread_fftwriter(startFFTWriter,&stream_a,&stream_b);

    thread_ffta.join();
    thread_fftb.join();
    thread_stream.join();
    thread_fftwriter.join();
}

