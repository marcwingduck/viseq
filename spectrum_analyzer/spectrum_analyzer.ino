// led matrix //////////////////////////////////////////////////////////////////

#include <Adafruit_GFX.h>   // adafruit graphics library
#include <RGBmatrixPanel.h> // hardware-specific library

#define CLK 8
#define LAT A3
#define OE  9
#define A   A0
#define B   A1
#define C   A2
RGBmatrixPanel matrix(A, B, C, CLK, LAT, OE, false);

// colors
const uint16_t off = matrix.Color444(0, 0, 0);
uint16_t color1 = matrix.Color444(0, 2, 2);
uint16_t color2 = matrix.Color444(0, 3, 1);

const uint16_t bright = matrix.Color444(7, 7, 7);

// bin cols
int nCols = 2;

// frame
int frame = 0;

// logarithmic scale
bool lnScale = true;

// peak fall off
int fallOff = 500;

double appliedScale = 1.0;
long int averageMillis = 0;

// microphone & fft ////////////////////////////////////////////////////////////

#include <fix_fft.h>

#define M       6
#define N      (1<<M)

#define MIC_PIN A5

int micValue;
unsigned int micMin;
unsigned int micMax;

char re[64];
char im[64];

int amplitude;
char spectrum[32];

char peaks[32];
long int peaked[32];

int maxAmplitude = 0;

// button //////////////////////////////////////////////////////////////////////

#define BUTTON_PIN 13

bool buttonNow = HIGH;
bool buttonLast = HIGH;

long int downTime = -1;

// pot /////////////////////////////////////////////////////////////////////////

#define POT_PIN A4

int potValue = 0;

// general /////////////////////////////////////////////////////////////////////

unsigned int i, j;

// /////////////////////////////////////////////////////////////////////////////

void setup()
{
    analogReference(EXTERNAL);

    pinMode(BUTTON_PIN, INPUT);
    digitalWrite(BUTTON_PIN, HIGH); // enable pull-up

    matrix.begin();
    matrix.fillScreen(off); // clear

    char start = fallOff < 1001 ? 15 : 0;

    resetPeaks();
    resetScale();

    Serial.begin(9600);
}

void resetPeaks()
{
    char start = fallOff < 1001 ? 15 : 0;

    for (i = 0; i < 32; i++)
    {
        peaks[i] = start;

        peaked[i] = millis();
        matrix.drawPixel(i, 15 - start, color2);
        delay(13);
    }
}

void resetScale()
{
    averageMillis = millis();

    if (lnScale)
    {
        appliedScale = 15.0 / 10.0;
    }
    else
    {
        appliedScale = 15.0 / 181.0;
    }
}

// /////////////////////////////////////////////////////////////////////////////

void loop()
{
    micLoop();
    fftLoop();
    buttonLoop();
    potLoop();
    drawLoop();

    // progress frame
    frame = (frame + 1) % 32;
}

// /////////////////////////////////////////////////////////////////////////////

void micLoop()
{
    micMin = 1023;
    micMax = 0;

    // collect signals
    for (i = 0; i < N; i++)
    {
        // read DAC [0,1023]
        micValue = analogRead(MIC_PIN);

        if (micValue > 1023)
        {
            continue;
        }

        if (micValue > micMax)
        {
            micMax = micValue;
        }
        else if (micValue < micMin)
        {
            micMin = micValue;
        }

        // MAX9814 has an output of 2Vpp max on a 1.25V DC bias
        // lower edge: 0.25/(3.3/1024) = 77.57575757575758
        // upper edge: 2.25/(3.3/1024) = 698.1818181818182
        // map and clamp accordingly
        //micValue = map(micValue, 77, 698, 0, 1023);

        // convert to char range
        re[i] = (micValue / 4) - 128; // [-128,127]
        // set imaginary part to zero
        im[i] = 0;
    }

    int peak2peak = micMax - micMin;
    double volts = (peak2peak * 3.3) / 1024;

    //Serial.println(volts);
}

void fftLoop()
{
    // fft
    fix_fft(re, im, M, 0);

    // calculate amplitudes
    for (i = 0; i < N / 2; i++)
    {
        amplitude = re[i] * re[i] + im[i] * im[i]; // max 32768

        if (amplitude == 0)
        {
            spectrum[i] = 0;
            continue;
        }

        if (lnScale)
        {
            amplitude = log(amplitude); // [0,10]
            //map(amplitude, 0, 10, 0, 15);
        }
        else
        {
            amplitude = sqrt(amplitude); // [0,181]
            //map(amplitude, 0, 181, 0, 15);
        }

        if (amplitude > maxAmplitude)
        {
            maxAmplitude = amplitude;
        }

        // normalize values (scale to max)
        spectrum[i] = amplitude * appliedScale;
        //spectrum[i] = constrain(spectrum[i], 0, 15);
    }

    // reset max amplitude every 3 seconds
    if (millis() - averageMillis > 3000)
    {
        averageMillis = millis();
        appliedScale = 15.0 / (double) maxAmplitude;
        maxAmplitude = 0;
    }
}

void potLoop()
{
    // read DAC [0,1023]
    potValue = analogRead(POT_PIN);
    potValue = constrain(potValue, 0, 1023);

    //Serial.println(potValue);

    long hue1 = map(potValue, 0, 1023, 0, 1535);
    hue1 = constrain(hue1, 0, 1535);

    long hue2 = hue1 - 200;
    if (hue2 < 0)
    {
        hue2 = 1535 + hue2;
    }

    color1 = matrix.ColorHSV(hue1, 255, 127, true);
    color2 = matrix.ColorHSV(hue2, 255, 127, true);
}

void buttonLoop()
{
    buttonNow = digitalRead(BUTTON_PIN);

    if (buttonLast == HIGH && buttonNow == LOW)
    {
        // got pushed
        downTime = millis();
    }
    else if (buttonLast == LOW && buttonNow == HIGH)
    {
        // got released
        long int duration = millis() - downTime;
        //Serial.println(duration);

        if (duration > 1000)
        {
            resetPeaks();
        }
        else if (duration > 200)
        {
            lnScale = !lnScale;
            resetScale();
        }
        else
        {
            nCols = nCols == 1 ? 2 : 1;
        }
    }

    buttonLast = buttonNow;
}

void drawLoop()
{
    // consolidate bins
    if (nCols > 1)
    {
        for (i = 0; i < 32 / nCols; i++)
        {
            int avg = 0;
            for (j = 0; j < nCols; j++)
            {
                avg += spectrum[i * nCols + j];
            }
            avg /= nCols;

            for (j = 0; j < nCols; j++)
            {
                spectrum[i * nCols + j] = avg;
            }
        }
    }

    // draw
    for (int i = 0; i < 32; i++)
    {
        // clamp
        spectrum[i] = constrain(spectrum[i], 0, 15);

        // overwrite last avg if larger than last avg and more than one
        if (spectrum[i] > 0 && spectrum[i] > peaks[i])
        {
            peaks[i] = spectrum[i];
            peaked[i] = millis();
        }
        else if (peaks[i] >= 0 && millis() - peaked[i] > fallOff)
        {
            // reduce peak height incrementally after timeout
            peaks[i] -= 1;
        }

        // finally clear screen and draw bins
        for (j = 0; j < 16; j++)
        {
            if (j < spectrum[i]) // main color
            {
                matrix.drawPixel(i, 15 - j, color1);
            }
            else // clear
            {
                matrix.drawPixel(i, 15 - j, off);
            }
        }

        // draw peaks
        if (peaks[i] >= 0) // peak color
        {
            matrix.drawPixel(i, 15 - peaks[i], color2);
        }
    }
}
