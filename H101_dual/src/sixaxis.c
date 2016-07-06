/*
The MIT License (MIT)

Copyright (c) 2015 silverx

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


#include <inttypes.h>
#include "binary.h"
#include "sixaxis.h"
#include "drv_time.h"
//#include "drv_softi2c.h"
#include "util.h"
#include "config.h"
#include "led.h"
#include "drv_serial.h"

#include "drv_i2c.h"

#include <math.h>


void sixaxis_init(void)
{
// gyro soft reset      
	i2c_writereg(107, 128);

	delay(40000);
	
// clear sleep bit on old type gyro
	i2c_writereg(107, 1);


	int newboard = !(0x68 == i2c_readreg(117));

	i2c_writereg(27, 24);

	i2c_writereg(28, B00011000);	// 16G scale

// acc lpf for the new gyro type
//       0-6 ( same as gyro)
	if (newboard)
		i2c_writereg(29, ACC_LOW_PASS_FILTER);

// Gyro and acc DLPF low pass filter(old board)  or gyro only for the new board
	i2c_writereg(26, GYRO_LOW_PASS_FILTER);
}


int sixaxis_check(void)
{
	// read "who am I" register
	int id = i2c_readreg(117);

	return (0x78 == id || 0x68 == id || 0x7d == id);
}



float accel[3];
float gyro[3];

float accelcal[3];
float gyrocal[3];


float lpffilter(float in, int num);

void sixaxis_read(void)
{
	int data[16];

	int error = 0;

	float gyronew[3];

	error = (i2c_readdata(59, data, 14));
// 2nd attempt at an i2c read   
	if (error)
	  {
		  error = (i2c_readdata(59, data, 14));
		  // set a warning flag 
		  // not implemented
		  // warningflag = 1;
	  }

//if (error) return;

	accel[1] = (int16_t) ((data[0] << 8) + data[1]);
	accel[0] = -(int16_t) ((data[2] << 8) + data[3]);
	accel[2] = (int16_t) ((data[4] << 8) + data[5]);


	gyronew[1] = (int16_t) ((data[8] << 8) + data[9]);
	gyronew[0] = (int16_t) ((data[10] << 8) + data[11]);
	gyronew[2] = (int16_t) ((data[12] << 8) + data[13]);



	for (int i = 0; i < 3; i++)
	  {

		  gyronew[i] = gyronew[i] - gyrocal[i];

		  gyronew[i] = gyronew[i] * 0.061035156f * 0.017453292f;
#ifndef SOFT_LPF_NONE
		  gyro[i] = lpffilter(gyronew[i], i);
#else
		  gyro[i] = gyronew[i];
#endif
	  }

	float temp = gyro[0];
	gyro[0] = -gyro[1];
	gyro[2] = -gyro[2];
	gyro[1] = -temp;



}





void loadcal(void);

#define CAL_TIME 2e6

void gyro_cal(void)
{
	int data[6];

	unsigned long time = gettime();
	unsigned long timestart = time;
	unsigned long timemax = time;
	unsigned long lastlooptime = time;

	float gyro[3];
	float limit[3];

	for (int i = 0; i < 3; i++)
	  {
		  limit[i] = gyrocal[i];
	  }

// 2 and 15 seconds
	while (time - timestart < CAL_TIME && time - timemax < 15e6)
	  {

		  unsigned long looptime;
		  looptime = time - lastlooptime;
		  lastlooptime = time;
		  if (looptime == 0)
			  looptime = 1;

		  i2c_readdata(67, data, 6);

		  gyro[1] = (int16_t) ((data[0] << 8) + data[1]);
		  gyro[0] = (int16_t) ((data[2] << 8) + data[3]);
		  gyro[2] = (int16_t) ((data[4] << 8) + data[5]);

#ifdef OLD_LED_FLASH
		  if ((time - timestart) % 200000 > 100000)
		    {
			    ledon(B00000101);
			    ledoff(B00001010);
		    }
		  else
		    {
			    ledon(B00001010);
			    ledoff(B00000101);
		    }
#else
static int ledlevel = 0;
static int loopcount = 0;

loopcount++;
if ( loopcount>>5 )
{
	loopcount = 0;
	ledlevel = ledlevel + 1;
	ledlevel &=15;
}

if ( ledlevel > (loopcount&0xF) ) 
{
	ledon( 255);
}
else 
{
	ledoff( 255);
}
#endif
				
		  for (int i = 0; i < 3; i++)
		    {

			    if (gyro[i] > limit[i])
				    limit[i] += 0.1f;	// 100 gyro bias / second change
			    if (gyro[i] < limit[i])
				    limit[i] -= 0.1f;

			    limitf(&limit[i], 800);

			    if (fabsf(gyro[i]) > 100 + fabsf(limit[i]))
			      {
				      timestart = gettime();
							#ifndef OLD_LED_FLASH
							ledlevel = 1;
							#endif
			      }
			    else
			      {
				      lpf(&gyrocal[i], gyro[i], lpfcalc((float)looptime, 0.5 * 1e6));

			      }

		    }

		  while ((gettime() - time) < 1000)
			  delay(10);
		  time = gettime();

	  }



	if (time - timestart < CAL_TIME)
	  {
		  for (int i = 0; i < 3; i++)
		    {
			    gyrocal[i] = 0;

		    }

		  loadcal();
	  }


#ifdef SERIAL
	printf("gyro calibration  %f %f %f \n ", gyrocal[0], gyrocal[1], gyrocal[2]);
#endif

}


void acc_cal(void)
{
	accelcal[2] = 2048;
	for (int y = 0; y < 500; y++)
	  {
		  sixaxis_read();
		  for (int x = 0; x < 3; x++)
		    {
			    lpf(&accelcal[x], accel[x], 0.92);
		    }
			delay(1000);
		  gettime();	// if it takes too long time will overflow so we call it here

	  }
	accelcal[2] -= 2048;


	for (int x = 0; x < 3; x++)
	  {
		  limitf(&accelcal[x], 500);
	  }

}
