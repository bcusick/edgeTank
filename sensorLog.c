#include <Arduino_EdgeControl.h>

constexpr unsigned int adcResolution { 12 };

constexpr pin_size_t inputChannels [] {
    INPUT_420mA_CH01,   //tank 1 vol
    INPUT_420mA_CH02,   //tank 2 vol
    INPUT_05V_CH01,     //main flowmeter
    
    
};
constexpr size_t inputChannelsLen { sizeof(inputChannels) / sizeof(inputChannels[0]) };
int inputChannelIndex { 0 };

struct Voltages {
    float volt3V3;
    float volt5V;
};

constexpr size_t loops { 100 };
constexpr float toV { 3.3f / float { (1 << adcResolution) - 1 } };  //ADC conversion to Volts
constexpr float rDiv { 17.4f / ( 10.0f + 17.4f) };  //Voltage divider between MUX and ADC
int res420 = 220;

//tanks
constexpr float tankCross = 3.14 * 5.0**2; //TBD
constexpr float fullscale = 100;  //TBD

void setup()
{
    Serial.begin(9600);

    auto startNow = millis() + 2500;
    while (!Serial && millis() < startNow)
        ;

    delay(1000);
    Serial.println("Hello, Challenge!");

    Power.on(PWR_3V3);
    Power.on(PWR_VBAT);

    Wire.begin();
    Expander.begin();

    Serial.print("Waiting for IO Expander Initialization...");
    while (!Expander) {
        Serial.print(".");
        delay(100);
    }
    Serial.println(" done.");

    Input.begin();
    Input.enable();

    analogReadResolution(adcResolution);
}

void loop() {

    auto [ voltsMuxer, voltsInput ] = getAverageAnalogRead(inputChannels[inputChannelIndex]);
    switch (inputChannels[inputChannelIndex]) {
        case INPUT_420mA_CH01: 
            getTankVol(inputChannels[inputChannelIndex]);       //Tank1 Vol
            break;
        case INPUT_420mA_CH02: 
            getTankVol(inputChannels[inputChannelIndex]);       //Tank2 Vol 
            break;
        case INPUT_05V_CH01: 
            getFlow(inputChannels[inputChannelIndex]);          //Main Flowmeter
            break;
        
        
        default: break;
    }
    inputChannelIndex = ++inputChannelIndex % inputChannelsLen;
}

float getAverage5vRead(int pin){

    int tot { 0 };
    for (auto i = 0u; i < loops; i++)
        tot += Input.analogRead(pin);
    const auto avg = static_cast<float>(tot) * toV / static_cast<float>(loops) / rDiv;
    
    return (avg);
}

float getTankVol(int pin){ //TBD
    float value = getAverage5vRead(pin);
    float level = ((value / res420 * 1000.0) - 4.0) * 6.25;  //convert to ma,  x = (y - 4)/16 since 4 = 0 and 20 = max

    //psuedo 
    pressure = level * fullscale    //psi
    height = pressure * 2.31        //feet 
    volume = height * tankCross     //ft^3
    vol = volume * XX               //gallons

    return vol;
}

float getFlow(int pin){ //TBD
    float value = getAverage5vRead(pin)


    return flow;
}