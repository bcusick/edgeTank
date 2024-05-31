
constexpr unsigned int adcResolution { 12 };

constexpr pin_size_t inputChannels [] {
    INPUT_420mA_CH01,   //tank 1 vol
    INPUT_420mA_CH02,   //tank 2 vol
    
    
    
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
constexpr float tankCross = 3.14 * 5.0**2; //inches //TBD
constexpr float fullscale = 100;  //pressure sensor max //TBD
constexpr float bottomOffset = 5;  //inches //TBD

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

////////////

//tanks
constexpr float tankCross = 3.14 * 5.0**2; //inches //TBD
constexpr float fullscale = 100;  //pressure sensor max //TBD
constexpr float bottomOffset = 5;  //inches //TBD
float getTankVol(int pin){ //TBD
    float value = getAverage5vRead(pin);
    float level = ((value / res420 * 1000.0) - 4.0) * 6.25;  //convert to ma,  x = (y - 4)/16 since 4 = 0 and 20 = max

    //psuedo //TBD
    float pressure = level * fullscale  ;  //psi
    float height = pressure * 2.31 * 12 ;  //inch
    height -= bottomOffset              ;  //adjust for sensor location below tank bottom
    float volume = height * tankCross   ;  //in^3
    float vol = volume / 231            ;  //gallons

    return vol;
}

/////////////

constexpr float TANK_RADIUS_INCHES = 12.6f * 12f / 2f; //radius inches for 12.6' inside diamter tank
constexpr float TANK_CROSS_SECTION_AREA = 3.14f * TANK_RADIUS_INCHES * TANK_RADIUS_INCHES; // in^2
constexpr float PRESSURE_SENSOR_MAX = 100.0f;  // Maximum pressure sensor value in psi
constexpr float SENSOR_OFFSET_HEIGHT = 5.0f;  // Sensor offset from tank bottom in inches
constexpr float PSI_TO_HEIGHT_CONVERSION = 2.31f * 12.0f;  // Conversion factor from psi to inches of water
constexpr float INCHES_CUBIC_TO_GALLONS = 231.0f;  // Conversion factor from cubic inches to gallons
constexpr float CURRENT_LOOP_RESISTOR = 220.0f;  // 420ma current loop resistor


float getTankVolume(int pin) {
    float sensorValue = getAverage5vRead(pin);
    float current_mA = (sensorValue * 1000.0f / CURRENT_LOOP_RESISTOR);  // Convert sensor reading to mA 
    float pressure = (current_mA - 4.0f) * (PRESSURE_SENSOR_MAX / 16.0f);  // Convert mA to psi
    
    float waterHeight = pressure * PSI_TO_HEIGHT_CONVERSION;  // Convert psi to inches of water column
    waterHeight -= SENSOR_OFFSET_HEIGHT;  // Adjust for sensor location below tank bottom
    
    float volumeInCubicInches = waterHeight * TANK_CROSS_SECTION_AREA;  // Volume in cubic inches
    float volumeInGallons = volumeInCubicInches / INCHES_CUBIC_TO_GALLONS;  // Convert cubic inches to gallons
    
    return volumeInGallons;
}

///////////

float getFlow(int pin){ //TBD
    float value = getAverage5vRead(pin)


    return flow;
}