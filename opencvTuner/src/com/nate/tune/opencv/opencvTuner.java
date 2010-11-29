package com.nate.tune.opencv;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.Date;
import java.util.List;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.Intent;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.YuvImage;
import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.Window;
import android.widget.Toast;

import com.android.bluetooth.btComm.btComm;

public class opencvTuner extends Activity {
    private BluetoothAdapter mBluetoothAdapter = null;
    // Member object for the chat services
    private btComm mCommService = null;
    private Preview mPreview;
    private boolean mWantImg = false; 

    
    private static final int REQUEST_CONNECT_DEVICE = 1;
    private static final int REQUEST_ENABLE_BT = 2;
    
    public static final int MESSAGE_STATE_CHANGE = 1;
    public static final int MESSAGE_DATA_IN = 2;
    public static final int MESSAGE_TOAST = 5;
    
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		getWindow().setFlags(android.view.WindowManager.LayoutParams.FLAG_FULLSCREEN, android.view.WindowManager.LayoutParams.FLAG_FULLSCREEN);
		mPreview = new Preview( this );
		
		setContentView(mPreview);
		//setContentView(R.layout.main);
        //
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

	}
	
    @Override
    public void onStart() {
        super.onStart();
        Toast.makeText(getApplicationContext(), "Starting", Toast.LENGTH_LONG).show();
        // If BT is not on, request that it be enabled.
        // setupChat() will then be called during onActivityResult
        if (!mBluetoothAdapter.isEnabled()) {
            Intent enableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableIntent, REQUEST_ENABLE_BT);
        // Otherwise, setup the chat session
        } else {
            if (mCommService == null) setupComm();
        }
        

        
    }
    
    public OutputStream getOS() {
    	return mCommService.getOS();
    }
    
    public void sendData(byte[] data){
    	if(mCommService.getState() == mCommService.STATE_CONNECTED)
    		mCommService.write(data);
    }
    
    public boolean getWant() {
    	return mWantImg;
    }
    
    public void sentImage() {
    	mWantImg = false;
    }
    
    public boolean isConnected() {
    	return mCommService.getState() == mCommService.STATE_CONNECTED;
    }
    
    @Override
    public synchronized void onResume() {
        super.onResume();
 
        //Toast.makeText(getApplicationContext(), "Resuming", Toast.LENGTH_SHORT).show();
        // Performing this check in onResume() covers the case in which BT was
        // not enabled during onStart(), so we were paused to enable it...
        // onResume() will be called when ACTION_REQUEST_ENABLE activity returns.
        if (mCommService != null) {
            // Only if the state is STATE_NONE, do we know that we haven't started already
            if (mCommService.getState() == btComm.STATE_DISCONNECTED) {
              // Start the Bluetooth chat services
              // do nothing
            	
        			BluetoothDevice device = mBluetoothAdapter.getRemoteDevice("00:0D:F0:57:DA:05");
        			mCommService.connect(device);
        			//Toast.makeText(getApplicationContext(), "Connecting", Toast.LENGTH_SHORT).show();
        		
            }
        } else {
        	setupComm();
        }
        
    }
    
    private void setupComm() {
    	mCommService = new btComm(mHandler); 
		if(mCommService.getState() == mCommService.STATE_DISCONNECTED) {
			BluetoothDevice device = mBluetoothAdapter.getRemoteDevice("00:0D:F0:57:DA:05");
			mCommService.connect(device);

				
			//Toast.makeText(getApplicationContext(), "Connecting", Toast.LENGTH_SHORT).show();
		}
		
    }
    
    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
           switch (msg.what) {
            case MESSAGE_STATE_CHANGE:
                switch (msg.arg1) {
                case btComm.STATE_CONNECTED:
                	//TODO: disable button
                	Toast.makeText(getApplicationContext(), "connected!", Toast.LENGTH_LONG).show();
                    break;
                }
                
                break;
            case MESSAGE_TOAST:
            	if(msg.arg1 > 0) {
            		byte[] indata = (byte [])msg.obj;
            		String inmsg = new String(indata, 0, msg.arg1);            
            		Toast.makeText(getApplicationContext(), inmsg, Toast.LENGTH_LONG).show();
            	}
            	break;
            case MESSAGE_DATA_IN:
            	
            		//updateDataEdit((StringBuffer)msg.obj);
            	if(((String)msg.obj).equals("getImg")) {
            		mWantImg = true;
            		Toast.makeText(getApplicationContext(), "Want Image", Toast.LENGTH_LONG).show();
            	}
                break;
            }
        }
    };


}


class Preview extends SurfaceView implements SurfaceHolder.Callback {
	SurfaceHolder mHolder;
    Camera mCamera;
    ByteBuffer bBuf= null;
    int width=0, height=0;
    Context parent;
   
    
    Preview(Context context) {
        super(context);
        parent = context;
        // Install a SurfaceHolder.Callback so we get notified when the
        // underlying surface is created and destroyed.
        mHolder = getHolder();

        mHolder.addCallback(this);
        mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
    }

    public void surfaceCreated(SurfaceHolder holder) {
        // The Surface has been created, acquire the camera and tell it where
        // to draw.
        mCamera = Camera.open();
        try {
           mCamera.setPreviewDisplay(holder);
        } catch (IOException exception) {
            mCamera.release();
            mCamera = null;
            // TODO: add more exception handling logic here
        }
        mCamera.setPreviewCallbackWithBuffer(new PreviewCallback() {
			int fcount = 0;
			Date start = new Date();
			@Override
			public void onPreviewFrame(byte[] data, Camera camera) {
				opencvTuner par = (opencvTuner)parent;
				if(par.getWant() && par.isConnected()) {
				/*	Size pvs = camera.getParameters().getPreviewSize();
					YuvImage mImage = new YuvImage(data, camera.getParameters().getPreviewFormat(), pvs.width, pvs.height, null);
					ByteArrayOutputStream mBaos = new ByteArrayOutputStream();
					mImage.compressToJpeg( new Rect(0,0, width, height), 90, mBaos);
					par.sendData(("Length: " + mBaos.size()).getBytes());
					
					par.sendData(mBaos.toByteArray());

					*/
					try{
						Thread.sleep(300);
					} catch( InterruptedException e) {
						
					}
					par.sendData(("Length: " + data.length).getBytes());
					par.sendData(data.clone());
					
					par.sentImage();
				}
				fcount++;
				if (fcount % 10 == 0) {
					double ms = (new Date()).getTime() - start.getTime();
					Log.i("NativePreviewer", "fps:" + fcount / (ms / 1000.0));
					start = new Date();
					fcount = 0;
				}
				
				//par.sendData(("Framedata: " + fcount).getBytes());

				camera.addCallbackBuffer(data);
				
			}
		});
    }
   

    
    public void surfaceDestroyed(SurfaceHolder holder) {
        // Surface will be destroyed when we return, so stop the preview.
        // Because the CameraDevice object is not a shared resource, it's very
        // important to release it when the activity is paused.
        mCamera.stopPreview();
        mCamera.release();
        mCamera = null;
    }

    private Size getOptimalPreviewSize(List<Size> sizes, int w, int h) {
        final double ASPECT_TOLERANCE = 0.05;
        double targetRatio = (double) w / h;
        if (sizes == null) return null;

        Size optimalSize = null;
        double minDiff = Double.MAX_VALUE;

        int targetHeight = h;

        // Try to find an size match aspect ratio and size
        for (Size size : sizes) {
            double ratio = (double) size.width / size.height;
            if (Math.abs(ratio - targetRatio) > ASPECT_TOLERANCE) continue;
            if (Math.abs(size.height - targetHeight) < minDiff) {
                optimalSize = size;
                minDiff = Math.abs(size.height - targetHeight);
            }
        }

        // Cannot find the one match the aspect ratio, ignore the requirement
        if (optimalSize == null) {
            minDiff = Double.MAX_VALUE;
            for (Size size : sizes) {
                if (Math.abs(size.height - targetHeight) < minDiff) {
                    optimalSize = size;
                    minDiff = Math.abs(size.height - targetHeight);
                }
            }
        }
        return optimalSize;
    }

    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        // Now that the size is known, set up the camera parameters and begin
        // the preview.
        Camera.Parameters parameters = mCamera.getParameters();

        List<Size> sizes = parameters.getSupportedPreviewSizes();
        Size optimalSize = getOptimalPreviewSize(sizes, w, h);
        parameters.setPreviewSize(optimalSize.width, optimalSize.height);
        //parameters.setPreviewSize(640, 480);
        //parameters.setWhiteBalance(Camera.Parameters.WHITE_BALANCE_INCANDESCENT);
        parameters.setWhiteBalance(Camera.Parameters.WHITE_BALANCE_AUTO);
        Size cSize = parameters.getPreviewSize();
        this.width = cSize.width;
        this.height = cSize.height;
        byte[] buffer = new byte[(ImageFormat.getBitsPerPixel(mCamera.getParameters().getPreviewFormat())*w*h)/8];
        
        mCamera.addCallbackBuffer(buffer);
        
        
        mCamera.setParameters(parameters);
        mCamera.startPreview();
    }
}



