// led matrix //////////////////////////////////////////////////////////////////

#include <Adafruit_GFX.h>   // adafruit graphics library
#include <RGBmatrixPanel.h> // hardware-specific library

RGBmatrixPanel matrix(A0, A1, A2, 8, A3, 9, false);

const int ledCols = 32;
const int ledRows = 16;

// colors
const uint16_t off = matrix.Color444(0, 0, 0);
uint16_t color1 = matrix.Color444(0, 2, 2);
uint16_t color2 = matrix.Color444(0, 3, 1);

// frame
int frame = 0;

// bin cols
int binSize = 2;

// peak fall off
const int fallOff = 500;

double scale = 1.;
long int averageMillis = 0;

// microphone //////////////////////////////////////////////////////////////////

#define MIC_PIN A5

int micVal;

// fft /////////////////////////////////////////////////////////////////////////

#include <fix_fft.h>

#define M  6
#define N (1<<M)

char re[64];
char im[64];

char spectrum[32];
char peaks[32];
long int peakMillis[32];

int amplitude;
int maxAmplitude = 1;

// button //////////////////////////////////////////////////////////////////////

#define BUTTON_PIN 13

bool buttonVal = HIGH;
bool prevButtonVal = HIGH;

long int downTime = 0;

// pot /////////////////////////////////////////////////////////////////////////

#define POT_PIN A4

int potVal = 0;

// general /////////////////////////////////////////////////////////////////////

unsigned int i;
unsigned int j;

// /////////////////////////////////////////////////////////////////////////////

void setup()
{
    analogReference(EXTERNAL);

    pinMode(BUTTON_PIN, INPUT);
    digitalWrite(BUTTON_PIN, HIGH); // enable pull-up

    matrix.begin();
    matrix.fillScreen(off); // clear

    resetPeaks();

    Serial.begin(9600);
}

void resetPeaks()
{
    char start = ledRows;

    for (i = 0; i < ledCols; i++)
    {
        peaks[i] = start;
        peakMillis[i] = millis();
        matrix.drawPixel(i, ledRows - start, color2);
        delay(13);
    }
}

// /////////////////////////////////////////////////////////////////////////////

void loop()
{
    micLoop();
    fftLoop();
    buttonLoop();
    potLoop();

    // consolidate bins
    if (binSize > 1)
    {
        for (i = 0; i < ledCols / binSize; i++)
        {
            int avg = 0;
            for (j = 0; j < binSize; j++)
            {
                avg += spectrum[i * binSize + j];
            }
            avg /= binSize;

            for (j = 0; j < binSize; j++)
            {
                spectrum[i * binSize + j] = avg;
            }
        }
    }

    drawLoop();

    // reset max amplitude every second
    if (millis() - averageMillis > 1000)
    {
        averageMillis = millis();
        scale = ledRows / maxAmplitude;
        maxAmplitude = 1;
    }

    // progress frame
    frame = (frame + 1) % ledCols;
}

// /////////////////////////////////////////////////////////////////////////////

void micLoop()
{
    // collect mic signals
    for (i = 0; i < N; i++)
    {
        // read DAC [0,1023]
        micVal = analogRead(MIC_PIN);
        // convert to char range
        re[i] = (micVal / 4) - 128; // [-128,127]
        // set imaginary part to zero
        im[i] = 0;
    }
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

        // log scale [0,10]
        amplitude = log(amplitude);

        if (amplitude > maxAmplitude)
        {
            maxAmplitude = amplitude;
        }

        spectrum[i] = amplitude;
    }
}

void potLoop()
{
    // read DAC [0,1023]
    potVal = analogRead(POT_PIN);

    long hue1 = map(potVal, 0, 1023, 0, 1535);

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
    buttonVal = digitalRead(BUTTON_PIN);

    if (prevButtonVal == HIGH && buttonVal == LOW) // got pushed
    {
        downTime = millis();
    }
    else if (prevButtonVal == LOW && buttonVal == HIGH) // got released
    {
        if (millis() - downTime > 500)
        {
            binSize = binSize == 1 ? 2 : 1;
        }
        else
        {
            resetPeaks();
        }
    }

    prevButtonVal = buttonVal;
}

void drawLoop()
{
    for (i = 0; i < ledCols; i++)
    {
        char val = spectrum[i] * scale;
        if (val > ledRows)
        {
            val = ledRows;
        }

        if (val > peaks[i])
        {
            peaks[i] = val;
            peakMillis[i] = millis();
        }
        else if (peaks[i] > 0 && millis() - peakMillis[i] > fallOff)
        {
            // reduce peak height incrementally after timeout
            peaks[i] -= 1;
        }

        // clear screen and draw bins
        for (j = 0; j < ledRows; j++)
        {
            matrix.drawPixel(i, ledRows - 1 - j, j < val ? color1 : off);
        }

        // draw peaks
        if (peaks[i] > 0)
        {
            matrix.drawPixel(i, ledRows - peaks[i], color2);
        }
    }
}
