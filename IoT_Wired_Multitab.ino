#include <UIPEthernet.h>

#define AREFVOLTAGE 5.0

byte mac[] = { 0x00,0x0B,0xDC,0x1D,0x64,0xAE };
EthernetServer server = EthernetServer(80);

struct Relay8 {
	byte status;
	float latency;
} relay;

class ShiftArray {
private:
	int clkPin, latchPin, dataPin;
	byte data;

public:
	ShiftArray(int clkPin, int latchPin, int dataPin) {
		this->clkPin = clkPin;
		this->latchPin = latchPin;
		this->dataPin = dataPin;
		pinMode(clkPin, 1);
		pinMode(latchPin, 1);
		pinMode(dataPin, 1);
	}

	byte set(byte d) {
		digitalWrite(latchPin, 0);
		shiftOut(dataPin, clkPin, MSBFIRST, d);
		digitalWrite(latchPin, 1);
	}

	byte setWaitFor(byte d, void (*function)()) {
		digitalWrite(latchPin, 0);
		shiftOut(dataPin, clkPin, MSBFIRST, d);
		function();
		digitalWrite(latchPin, 1);
	}
} shifter(7,6,5);

class ACManager {
private:
	int adcPin;
	float divFactor;
	int vpp;
	uint32_t oldTime=0, nowTime=0, tmpTime=0;
	int oldV=0, nowV=0;

public:
	ACManager(int adcPin, float divFactor) {
		this->adcPin = adcPin;
		this->divFactor = divFactor;
	}

	int update() {
		oldV = nowV;
		nowV=analogRead(adcPin);
		if (nowV < oldV) {
			tmpTime = micros();
			oldTime = nowTime;
			nowTime = tmpTime;
			vpp = oldV;
			return 1;
		}
		return 0;
	}

	float getVPP() {
		return float(vpp)*AREFVOLTAGE*divFactor/1024.0;
	}

	float getFrequency() {
		return 1.0 / (nowTime - oldTime);
	}

	
} acm(A0,100);

void setup() {
	Serial.begin(115200);
	Serial.print(F("\r\nSerial ready."));

	Serial.print(F("\r\nInitializing..."));
	Serial.print(F("OK"));

	Serial.print(F("\r\nBeginning ethernet module..."));
	if (!Ethernet.begin(mac)) {
		Serial.print(F("Failed"));
		Serial.print(F("\r\nPlease restart Arduino"));
	}
	Serial.print(F("OK"));

	Serial.print(F("\r\nStarting server..."));

	server.begin();
	Serial.print(Ethernet.localIP());
	Serial.print(F("...OK"));

}

void loop() {
	EthernetClient client = server.available();
	acm.update();
	if (client) {
		char c;
		char buffer[50] = { 0 };
		char req[7] = { 0 };
		char url[20] = { 0 };
		int idx = 0;
		Serial.print(F("\r\n--------------------"));
		Serial.print(F("\r\nNew client"));
		Serial.print(F("\r\nNumber of bytes to read: "));
		Serial.print(client.available());
		Serial.print(F("\r\n<<<Following Message>>>\r\n"));
		while ((c = client.read()) != ' ') req[idx++] = c;
		idx = 0;
		while ((c = client.read()) != ' ') url[idx++] = c;
		Serial.println(req);
		Serial.println(url);

		if (!strcmp(req, "GET")) {
			if (!strcmp(url, "/")) {
				client.println(F("HTTP/1.1 200 OK"));
				client.println(F("Content-type: text/html\n"));
				client.print(F("<!DOCTYPE html><title>IoT Multitab Va1R0</title><meta content=\"width=device-width,initial-scale=1,maximum-scale=1,minimum-scale=1,user-scalable=no,target-densitydpi=medium-dpi\"name=viewport><style>*{margin:0;padding:0}body,html{height:100%}body{font-family:Helvetica,sans-serif}li{list-style:none}a{test-decoration:none}#header{height:45px;background-color:#00bfff}#header h1{text-align:center;line-height:45px}#acm{text-align:center;background-color:#e9e9e9}#sw_area{width:900px;margin:0 auto;overflow:hidden}.sw{width:225px;height:auto;text-align:center;float:left;transition-duration:.3s}.sw>div{width:175px;height:175px;border-radius:50px;background-color:grey;margin:10px 25px;left:20px}.sw>div.on{background-color:#0f0}.sw>div.off{background-color:green}#def{position:absolute;display:none;width:100%;height:100%}.zoom-3-5{zoom:.6;-webkit-transform:scale(.6)}</style><script crossorigin=anonymous integrity=\"sha256-hVVnYaiADRTO2PzUGmuLJr8BLUSjGIZsDYGmIJLv2b8=\"src=https://code.jquery.com/jquery-3.1.1.min.js></script><script>$(function(){function s(){$.get(\"/get_stat\",function(s){var t=s.split(\",\");e=parseInt(t[2]),$(\"#volt\").html(t[0]),$(\"#freq\").html(t[1]);for(var a=0;a<8;a++){var i=$(\".sw\").eq(a).children();e&1<<a?(i.eq(0).removeClass(\"off\"),i.eq(0).addClass(\"on\"),i.eq(2).html(\"ON\")):(i.eq(0).removeClass(\"on\"),i.eq(0).addClass(\"off\"),i.eq(2).html(\"OFF\"))}})}function t(s,t){$.post(\"/set_stat\",s+\",\"+t)}for(var e=0,a=1;a<=8;a++){var i='<div class=\"sw\" nth='+a+\" ><div></div><h1>Switch\"+a+\"</h1><h2>load...</h2></div>\";$(\"#sw_area\").html($(\"#sw_area\").html()+i)}var n=window.navigator.userAgent;(/lgtelecom/i.test(n)||/Android/i.test(n)||/blackberry/i.test(n)||/iPhone/i.test(n)||/ipad/i.test(n)||/samsung/i.test(n)||/symbian/i.test(n)||/sony/i.test(n)||/SCH-/i.test(n)||/SPH-/i.test(n))&&$(\".sw\").addClass(\"zoom-3-5\"),$(window).resize(function(){var s=parseInt($(\".sw\").css(\"width\")),t=Math.floor($(window).width()/s);t>4?t=4:0,$(\"#sw_area\").css(\"width\",s*t)}).resize(),$(\".sw>div\").click(function(){var e=$(\"#def\"),a=$(this);e.css(\"display\",\"block\"),t(a.parent().attr(\"nth\"),a.hasClass(\"on\")?\"0\":a.hasClass(\"off\")?\"1\":\"\"),s(),e.css(\"display\",\"none\")}),setInterval(function(){s()},1e3)})</script><div id=wrap><header id=header><h1>IoT Multitab</h1></header><div id=acm><span>Voltage: </span><span id=volt>load...</span><span>, Freq: </span><span id=freq>load...</span></div><div id=sw_area></div><div id=def></div></div>"));
			}
			else if (!strcmp(url, "/get_stat")) {
				client.println(F("HTTP/1.1 200 OK"));
				client.print("\r\n");
				client.print(acm.getVPP());
				client.print(',');
				client.print(acm.getFrequency());
				client.print(',');
				client.print(relay.status);
				Serial.print(relay.status);
			}
			
			else client.print(F("HTTP/1.1 404 NOT FOUND\r\n"));
		}

		else if (!strcmp(req, "POST")) {
			if (!strcmp(url, "/set_stat")) {
				while (client.available()) if (client.read() == '\n') if (client.read() == '\r') if (client.read() == '\n') break;
				idx = 0;
				while (client.available()) buffer[idx++] = client.read();
				buffer[idx] = '\0';
				Serial.print(buffer);
				byte mask = 1 << atoi(strtok(buffer, ",")) - 1;
				if (atoi(strtok(NULL, "."))) relay.status |= mask;
				else relay.status &= ~mask;
				shifter.set(relay.status);
			}
		}
		


		client.stop();
	}
}
