/*
 ____  _____ _        _    
| __ )| ____| |      / \   
|  _ \|  _| | |     / _ \  
| |_) | |___| |___ / ___ \ 
|____/|_____|_____/_/   \_\

The platform for ultra-low latency audio and sensor processing

http://bela.io

A project of the Augmented Instruments Laboratory within the
Centre for Digital Music at Queen Mary University of London.
http://www.eecs.qmul.ac.uk/~andrewm

(c) 2016 Augmented Instruments Laboratory: Andrew McPherson,
  Astrid Bin, Liam Donovan, Christian Heinrichs, Robert Jack,
  Giulio Moro, Laurel Pardue, Victor Zappi. All rights reserved.

The Bela software is distributed under the GNU Lesser General Public License
(LGPL 3.0), available here: https://www.gnu.org/licenses/lgpl-3.0.txt
*/

#include <Bela.h>
#include <cmath>
#include <OSCServer.h>
#include <OSCClient.h>

OSCServer oscServer;
OSCClient oscClient;

int localPort = 7562;
int remotePort = 7563;
const char* remoteIp = "192.168.7.1";

float gOSCSendRate = 500; // milliseconds
float gOSCSendCount = 0;

// Set range for analog outputs designed for driving LEDs
const float kMinimumAmplitude = (1.5 / 5.0);
const float kAmplitudeRange = 1.0 - kMinimumAmplitude;

// Analog IOs
const int gRedLEDPin = 0; // Analog Output 0
const int gGreenLEDPin = 1; // Analog Output 1
const int gPressureSensorPin = 0; // Analog Input 1
float gInverseSampleRate;
float gMillisecondsPerAnalogSample;
float gRedLEDFrequency = 3.0;
float gRedLEDAmplitude;
float gRedLEDPhase;
float gGreenLEDFrequency = 3.0;
float gGreenLEDAmplitude;
float gGreenLEDPhase;
float gPressureValue;

// parse messages received by OSC Server
// msg is Message class of oscpkt: http://gruntthepeon.free.fr/oscpkt/
int parseMessage(oscpkt::Message msg){
    
    rt_printf("received message to: %s\n", msg.addressPattern().c_str());
    
    int intArg;
    float floatArg;
    if (msg.match("/osc-test").popInt32(intArg).popFloat(floatArg).isOkNoMoreArgs()){
        rt_printf("received int %i and float %f\n", intArg, floatArg);
    }
    float analogOutput0Amp;
    float analogOutput0Freq;
    if (msg.match("/bela/analogOutputs/0").popFloat(analogOutput0Amp).popFloat(analogOutput0Freq).isOkNoMoreArgs()){
    	gRedLEDAmplitude = analogOutput0Amp;
    	gRedLEDFrequency = analogOutput0Freq;
    }
    float analogOutput1Amp;
    float analogOutput1Freq;
    if (msg.match("/bela/analogOutputs/1").popFloat(analogOutput1Amp).popFloat(analogOutput1Freq).isOkNoMoreArgs()){
    	gGreenLEDAmplitude = analogOutput1Amp;
    	gGreenLEDFrequency = analogOutput1Freq;
    }
    return intArg;
}

// Convert milliseconds to samples
float millisToAnalogSamples(float milliseconds) {
	return milliseconds * gMillisecondsPerAnalogSample;
}

bool setup(BelaContext *context, void *userData)
{
    oscServer.setup(localPort);
    oscClient.setup(remotePort, remoteIp);
    
    // the following code sends an OSC message to address /osc-setup
    // then waits 1 second for a reply on /osc-setup-reply
    bool handshakeReceived = false;
    oscClient.sendMessageNow(oscClient.newMessage.to("/osc-setup").end());
    oscServer.receiveMessageNow(1000);
    while (oscServer.messageWaiting()){
        if (oscServer.popMessage().match("/osc-setup-reply")){
            handshakeReceived = true;
        }
    }
    
    if (handshakeReceived){
        rt_printf("handshake received!\n");
    } else {
        rt_printf("timeout!\n");
    }

    gMillisecondsPerAnalogSample = context->analogSampleRate / 1000;
    gInverseSampleRate = 1.0 / context->analogSampleRate;
    gOSCSendRate = millisToAnalogSamples(gOSCSendRate); // Convert from ms to analog samples
    rt_printf("OSC Send Rate: %f", gOSCSendRate);
    
	return true;
}

void render(BelaContext *context, void *userData)
{
    // receive OSC messages, parse them, and send back an acknowledgment
    while (oscServer.messageWaiting()){
        int count = parseMessage(oscServer.popMessage());
    }
    // send OSC messages every gOSCSendRate number of samples
    if (++gOSCSendCount >= gOSCSendRate) {
    	oscClient.queueMessage(oscClient.newMessage.to("/bela/analogInputs/0").add(gPressureValue).end());
    	gOSCSendCount = 0;
    }

    // Analog inputs and outputs
    for(unsigned int n = 0; n < context->analogFrames; n++) {
		// Set LED to different phase for each analog output channel
		float redOut   = kMinimumAmplitude + kAmplitudeRange * 0.5f * (1.0f + sinf(gRedLEDPhase))  * gRedLEDAmplitude;
		float greenOut = kMinimumAmplitude + kAmplitudeRange * 0.5f * (1.0f + sinf(gGreenLEDPhase))  * gGreenLEDAmplitude;
		// Write analog outputs
		analogWrite(context, n, gRedLEDPin,   redOut);
		analogWrite(context, n, gGreenLEDPin, greenOut);
        // Update and wrap phase of sine tone
		gRedLEDPhase   += 2.0f * (float)M_PI * gRedLEDFrequency   * gInverseSampleRate;
		gGreenLEDPhase += 2.0f * (float)M_PI * gGreenLEDFrequency * gInverseSampleRate;
		if(gRedLEDPhase   > M_PI) gRedLEDPhase   -= 2.0f * (float)M_PI;
		if(gGreenLEDPhase > M_PI) gGreenLEDPhase -= 2.0f * (float)M_PI;
		// Read analog inputs
		gPressureValue = analogRead(context, n, gPressureSensorPin);
	}
}

void cleanup(BelaContext *context, void *userData)
{

}

/*
\example analog-io/render.cpp

This example is designed to demo Bela communicating with PaperPrograms: https://github.com/acarabott/paperprograms/tree/bela

This example combines the OSC, analog input and output examples.

It assumes red and green LEDs connected to analog outputs 1 and 2, and a pressure sensor connected to analog input 1.

The amplitude and frequency of the LEDs are exposed via the OSC path `/bela/analogOutputs/n float amp, float freq`.

The pressure sensor is sent every gOSCSendRate number of samples to `/bela/analogInputs/0 float value`.

*/