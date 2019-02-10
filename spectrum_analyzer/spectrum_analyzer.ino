#include <Adafruit_GFX.h>   // adafruit graphics library
#include <RGBmatrixPanel.h> // hardware-specific library
#include <fix_fft.h>        // fft

// pins
#define MIC_PIN A5
#define BUTTON_PIN 13
#define POT_PIN A4

// matrix size
#define LED_COLS 32
#define LED_ROWS 16

// matrix and colors
RGBmatrixPanel matrix(A0, A1, A2, 8, A3, 9, false);
const uint16_t color0 = matrix.Color444(0, 0, 0);
uint16_t color1 = color0;
uint16_t color2 = color0;

// fft
#define M  6
#define N (1<<M) // 64
char re[N];
char im[N];

// N/2 must match LED_COLS!
float spectrum[N / 2];
char peaks[N / 2];
long int peakMillis[N / 2];

// dynamic scaling
double scale = 1.;
long int averageMillis = 0;
float maxAmpNorm = 0.;
float normAmp = log(32768);

// button
bool buttonVal = HIGH;
bool prevButtonVal = HIGH;
long int pushTime = 0;

// counter
unsigned int i = 0;
unsigned int j = 0;

// visualization settings
int binSize = 1; // two column bin mode
const int fallOff = 500; // peak fall off in ms

// /////////////////////////////////////////////////////////////////////////////

void setup()
{
    analogReference(EXTERNAL);
    pinMode(BUTTON_PIN, INPUT);
    digitalWrite(BUTTON_PIN, HIGH); // enable pull-up

    matrix.begin();

    // read initial colors
    potLoop();
    resetPeaks();

    Serial.begin(9600);
}

void resetPeaks()
{
    char start = LED_ROWS;

    matrix.fillScreen(color0); // clear
    for (i = 0; i < LED_COLS; i++)
    {
        peaks[i] = start;
        peakMillis[i] = millis();
        matrix.drawPixel(i, LED_ROWS - start, color2);
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
    if (binSize > 0)
    {
        int binWidth = 1 << binSize;

        for (i = 0; i < LED_COLS / binWidth; i++)
        {
            float maxVal = 0.;
            for (j = 0; j < binWidth; j++)
            {
                float val = spectrum[i * binWidth + j];
                maxVal = max(maxVal, val);
            }

            for (j = 0; j < binWidth; j++)
            {
                spectrum[i * binWidth + j] = maxVal;
            }
        }
    }

    drawLoop();

    // reset max amplitude every second
    if (millis() - averageMillis > 1000)
    {
        averageMillis = millis();
        scale = 1.;
        if (maxAmpNorm > 0.16) // silence returns values of up to 0.15
        {
            scale = (1. / maxAmpNorm) * LED_ROWS;
        }
        maxAmpNorm = 0.;
    }
}

// /////////////////////////////////////////////////////////////////////////////

void micLoop()
{
    // collect mic signals
    for (i = 0; i < N; i++)
    {
        // read DAC map from [0,1023] to [-128,127]
        re[i] = analogRead(MIC_PIN) / 4 - 128;
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
        spectrum[i] = 0.;

        int amp = re[i] * re[i] + im[i] * im[i]; // max 32768

        if (amp > 0)
        {
            // log scale [0., 10.3972] normalized to [0.,1.]
            float ampNorm = log(amp) / normAmp;
            maxAmpNorm = max(maxAmpNorm, ampNorm);
            spectrum[i] = ampNorm;
        }
    }
}

void potLoop()
{
    // read DAC [0,1023]
    int potVal = analogRead(POT_PIN);

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
        pushTime = millis();
    }
    else if (prevButtonVal == LOW && buttonVal == HIGH) // got released
    {
        if (millis() - pushTime > 500) // long push
        {
            resetPeaks();
        }
        else // short push
        {
            binSize = (binSize + 1) % 3;
        }
    }

    prevButtonVal = buttonVal;
}

void drawLoop()
{
    for (i = 0; i < LED_COLS; i++)
    {
        char val = round(spectrum[i] * scale);
        val = min(val, LED_ROWS);

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
        for (j = 0; j < LED_ROWS; j++)
        {
            matrix.drawPixel(i, LED_ROWS - 1 - j, j < val ? color1 : color0);
        }

        // draw peaks
        if (peaks[i] > 0)
        {
            matrix.drawPixel(i, LED_ROWS - peaks[i], color2);
        }
    }
}
