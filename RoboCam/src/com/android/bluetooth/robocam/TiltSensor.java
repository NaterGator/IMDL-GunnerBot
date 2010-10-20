package com.android.bluetooth.robocam;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;


public class TiltSensor {

	private SensorManager mSensorManager;
	private RoboCamComm mCommService;
	private int mSpeedL;
	private int mSpeedR;
	private String mDir;
	public boolean disable = true;
	
	public TiltSensor(Context ctex, RoboCamComm commService) {
		this.mCommService = commService;
		mSensorManager = (SensorManager) ctex.getSystemService(Context.SENSOR_SERVICE);
		mSensorManager.registerListener(listener, mSensorManager.getDefaultSensor(Sensor.TYPE_ORIENTATION), SensorManager.SENSOR_DELAY_GAME);
	}
	
	private SensorEventListener listener = new SensorEventListener() {

		@Override
		public void onAccuracyChanged(Sensor sensor, int accuracy) {
			// TODO Auto-generated method stub
			
		}

		@Override
		public void onSensorChanged(SensorEvent event) {
			if(event.sensor.getType() == Sensor.TYPE_ORIENTATION && disable == false ) {
				/*
				 * All values are angles in degrees.
					values[0]: Azimuth, angle between the magnetic north direction and the Y axis, around the Z axis (0 to 359). 0=North, 90=East, 180=South, 270=West
					values[1]: Pitch, rotation around X axis (-180 to 180), with positive values when the z-axis moves toward the y-axis.
					values[2]: Roll, rotation around Y axis (-90 to 90), with positive values when the x-axis moves toward the z-axis.
				 */
				
				float leftRight = event.values[1]; //positive 90 turning left
				float frontBack = event.values[2]; //negative 90 moving forward
				int speedL;
				int speedR;
				String dir;
				float lInhibit=1;
				float rInhibit=1;
				int angleLimit = 35;
				
				if(Math.abs(leftRight) > angleLimit)
					leftRight = angleLimit * Math.signum(leftRight);
				if(Math.abs(frontBack) > angleLimit)
					frontBack = angleLimit * Math.signum(frontBack);
				
				if(frontBack <= 0)
					dir = "F";
				else
					dir = "R";
				
				float speedFactor =  Math.abs(frontBack / angleLimit);
				
				if(leftRight > 0)
					//turning right
					lInhibit = 1 - Math.abs(leftRight / angleLimit);
				else 
					rInhibit = 1 - Math.abs(leftRight / angleLimit);

				speedL = (int)  (9000 * (speedFactor * lInhibit));
				speedR = (int)  (9000 * (speedFactor * rInhibit));
				
				if(mSpeedL != speedL) {
					mSpeedL = speedL;
					mCommService.write( ("$FB$setL|" + mSpeedL + "$FE$ ").getBytes() );
				}
				if(mSpeedR != speedR) {
					mSpeedR = speedR;
					mCommService.write( ("$FB$setR|" + mSpeedR + "$FE$ ").getBytes() );
				}
				if(mDir != dir) {
					mDir = dir;
					mCommService.write( ("$FB$dirL|" + dir + "$FE$  $FB$dirR|" + dir + "$FE$ ").getBytes() );
				}
				//mCommService.write( ("$FB$setL|" + event.values[1] + "$FE$  $FB$setR|" + event.values[2] + "$FE$ " ).getBytes() );
			}
			
		}
		
	};
	
}