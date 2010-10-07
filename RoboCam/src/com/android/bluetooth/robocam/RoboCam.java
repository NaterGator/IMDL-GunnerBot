package com.android.bluetooth.robocam;

import java.io.IOException;
import java.util.List;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;


public class RoboCam extends Activity {
	
    // Message types sent from the BluetoothChatService Handler
    public static final int MESSAGE_STATE_CHANGE = 1;
    public static final int MESSAGE_DATA_IN = 2;
    //public static final int MESSAGE_WRITE = 3;
    //public static final int MESSAGE_DEVICE_NAME = 4;
    public static final int MESSAGE_TOAST = 5;

    
    // Key names received from the BluetoothChatService Handler
    public static final String DEVICE_NAME = "device_name";
    public static final String TOAST = "toast";
    
    // Layout Views
    //private ListView mConversationView
    private Button mConnectButton;
    
    private TextView mStatusText;
    private TextView mDebugText;
    
    private static final int REQUEST_CONNECT_DEVICE = 1;
    private static final int REQUEST_ENABLE_BT = 2;
    
    private String mConnectedDeviceName = null;
    // Array adapter for the conversation thread
    //private ArrayAdapter<String> mConversationArrayAdapter;
    // String buffer for outgoing messages
    // Local Bluetooth adapter
    private BluetoothAdapter mBluetoothAdapter = null;
    // Member object for the chat services
    private RoboCamComm mCommService = null;
    private PreviewSurface mPreview;
	private V1DisplayProcessor mDisplayProc = null;
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		
		mPreview = new PreviewSurface( this );
		
		setContentView(mPreview);
		
        // Get local Bluetooth adapter
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        mDisplayProc = new V1DisplayProcessor(this);

	}
	
    @Override
    public void onStart() {
        super.onStart();

        // If BT is not on, request that it be enabled.
        // setupChat() will then be called during onActivityResult
     /*   if (!mBluetoothAdapter.isEnabled()) {
            Intent enableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableIntent, REQUEST_ENABLE_BT);
        // Otherwise, setup the chat session
        } else {
            if (mCommService == null) setupComm();
        }*/
    }
    
    @Override
    public synchronized void onResume() {
        super.onResume();
 

        // Performing this check in onResume() covers the case in which BT was
        // not enabled during onStart(), so we were paused to enable it...
        // onResume() will be called when ACTION_REQUEST_ENABLE activity returns.
        if (mCommService != null) {
            // Only if the state is STATE_NONE, do we know that we haven't started already
            if (mCommService.getState() == RoboCamComm.STATE_DISCONNECTED) {
              // Start the Bluetooth chat services
              // do nothing
            }
        }
    }
   
    private void setupComm() {


      

        mStatusText = (TextView) findViewById(R.id.conn_status);
        mDebugText = (TextView) findViewById(R.id.debug_out);
        // Initialize the send button with a listener that for click events
        mConnectButton = (Button) findViewById(R.id.button_connect);
        
        
        mConnectButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                // Establish BT connection
            	if(mCommService.getState()==RoboCamComm.STATE_DISCONNECTED) {
            		//TODO: figure out how to get device
            		BluetoothDevice device = mBluetoothAdapter.getRemoteDevice("00:06:66:03:16:E6");
            		mCommService.connect(device);
            	} else if(mCommService.getState() == RoboCamComm.STATE_CONNECTED) {
            		mCommService.stop();
            	}
            }
        });


        // Initialize the BluetoothChatService to perform bluetooth connections
        
        mCommService = new RoboCamComm(this, mHandler);
        
    }
    
    // The Handler that gets information back from the BluetoothChatService
    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MESSAGE_STATE_CHANGE:
                switch (msg.arg1) {
                case RoboCamComm.STATE_CONNECTED:
                	//TODO: disable button
                	mStatusText.setText("Connected.");
                	mConnectButton.setText("Disconnect.");
                	mConnectButton.setClickable(true);
                    break;
                case RoboCamComm.STATE_CONNECTING:
                	//Do nothing
                	mStatusText.setText("Connecting...");
                	mConnectButton.setClickable(false);                    
                    break;
                case RoboCamComm.STATE_DISCONNECTED:
                	//TODO: Enable button
                	mStatusText.setText("Disconnected.");
                	mConnectButton.setText("Connect");
                	mConnectButton.setClickable(true);
                    break;
                }
                break;
            case MESSAGE_DATA_IN:
            		updateDataEdit((StringBuffer)msg.obj);
            		mDisplayProc.readNewSequence((StringBuffer)msg.obj);
                	mDisplayProc.updateDisplay();
                break;
            case MESSAGE_TOAST:
                Toast.makeText(getApplicationContext(), msg.getData().getString(TOAST),
                               Toast.LENGTH_SHORT).show();
                break;
            }
        }
    };

    
    //TODO: Delete this?
    public synchronized void updateDataEdit(StringBuffer szData) {
    	String sData = szData.toString();
    	mDebugText.setText((CharSequence)sData.concat(" Strlen: ".concat(Integer.toString(sData.length()))));
       
    }
    
    @Override
    public synchronized void onPause() {
        super.onPause();
    }

    @Override
    public void onStop() {
        super.onStop();
    }
    
    @Override
    public void onDestroy() {
        super.onDestroy();
        // Stop the Bluetooth chat services
        if (mCommService != null) mCommService.stop();
    }
}



class PreviewSurface extends SurfaceView implements SurfaceHolder.Callback {
	SurfaceHolder	mHolder;
	Camera 			mCamera;
	
    static { 
        //System.loadLibrary( "imageprocessing" );
        System.loadLibrary( "yuv420sp2rgb" );       
    } 
    
    /**
     * native function, that converts a byte array from ycbcr420 to RGB
     * @param in
     * @param width
     * @param height
     * @param textureSize
     * @param out
     */
    private native void yuv420sp2rgb(byte[] in, int width, int height, int textureSize, byte[] out);

	public PreviewSurface(Context context) {
		super(context);
		mHolder = getHolder();
		mHolder.addCallback( this );
		mHolder.setType( SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS );
	}
	
	public void surfaceCreated(SurfaceHolder holder) {
		mCamera = Camera.open();
		try{
			mCamera.setPreviewDisplay( mHolder );
			
			mCamera.setPreviewCallbackWithBuffer(new PreviewCallback() {
				public synchronized void onPreviewFrame(byte[] data, Camera camera) {
					Camera.Parameters params = camera.getParameters();
					int w = params.getPreviewSize().width; 
					int h = params.getPreviewSize().height;
					byte[] rgbinfo = new byte[w*h*3];
					yuv420sp2rgb(data, w, h, 256, rgbinfo);
					camera.addCallbackBuffer(data);
				}
				
			});
		} catch (IOException exception){
			mCamera.release();
			mCamera = null;
			
			//TODO: Add logic to handle camera acquisition failure
		}
			
	}
	
	public void surfaceDestroyed (SurfaceHolder holder) {
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
        List<Integer> pformats = parameters.getSupportedPictureFormats();
        if(pformats.contains(ImageFormat.RGB_565))
        	parameters.setPreviewFormat(ImageFormat.RGB_565);
        
        byte[] buffer = new byte[(ImageFormat.getBitsPerPixel(parameters.getPreviewFormat())*w*h)/8];
        mCamera.addCallbackBuffer(buffer);
        buffer = new byte[(ImageFormat.getBitsPerPixel(parameters.getPreviewFormat())*w*h)/8];
        mCamera.addCallbackBuffer(buffer);
        mCamera.setParameters(parameters);
        mCamera.startPreview();
    }
    
    
}