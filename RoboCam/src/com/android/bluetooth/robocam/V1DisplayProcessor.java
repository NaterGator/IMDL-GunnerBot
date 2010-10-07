package com.android.bluetooth.robocam;

import java.util.StringTokenizer;

import com.android.bluetooth.robocam.R;

import android.widget.ImageView;

public class V1DisplayProcessor {
	
	//Counter view references
	private ImageView counter_a;
	private ImageView counter_b;
	private ImageView counter_c;
	private ImageView counter_d;
	private ImageView counter_e;
	private ImageView counter_f;
	private ImageView counter_g;
	private ImageView counter_dp;
	
	//Signal Strength references
	private ImageView signal_1;
	private ImageView signal_2;
	private ImageView signal_3;
	private ImageView signal_4;
	private ImageView signal_5;
	private ImageView signal_6;
	private ImageView signal_7;
	private ImageView signal_8;
	
	//Signal indicator view references
	private ImageView band_laser;
	private ImageView band_ka;
	private ImageView band_k;
	private ImageView band_x;
	
	//Arrow view references
	private ImageView arrow_fwd;
	private ImageView arrow_side;
	private ImageView arrow_back;
	
	private boolean isDetectorOffline;
	private StringTokenizer displayStr;
	private ImageView[]	streamseq;
	
	public V1DisplayProcessor(RoboCam context) {
    	counter_a = (ImageView) context.findViewById(R.id.bar_a_lit);
    	counter_b = (ImageView) context.findViewById(R.id.bar_b_lit);
    	counter_c = (ImageView) context.findViewById(R.id.bar_c_lit);
    	counter_d = (ImageView) context.findViewById(R.id.bar_d_lit);
    	counter_e = (ImageView) context.findViewById(R.id.bar_e_lit);
    	counter_f = (ImageView) context.findViewById(R.id.bar_f_lit);
    	counter_g = (ImageView) context.findViewById(R.id.bar_g_lit);
    	counter_dp = (ImageView) context.findViewById(R.id.dpdot_lit);
    	
    	signal_1 = (ImageView) context.findViewById(R.id.sdot_lit_1);
    	signal_2 = (ImageView) context.findViewById(R.id.sdot_lit_2);
    	signal_3 = (ImageView) context.findViewById(R.id.sdot_lit_3);
    	signal_4 = (ImageView) context.findViewById(R.id.sdot_lit_4);
    	signal_5 = (ImageView) context.findViewById(R.id.sdot_lit_5);
    	signal_6 = (ImageView) context.findViewById(R.id.sdot_lit_6);
    	signal_7 = (ImageView) context.findViewById(R.id.sdot_lit_7);
    	signal_8 = (ImageView) context.findViewById(R.id.sdot_lit_8);
    	
    	band_laser = (ImageView) context.findViewById(R.id.ldot_lit);
    	band_ka = (ImageView) context.findViewById(R.id.kadot_lit);
    	band_k = (ImageView) context.findViewById(R.id.kdot_lit);
    	band_x = (ImageView) context.findViewById(R.id.xdot_lit);
    	
    	arrow_fwd = (ImageView) context.findViewById(R.id.arrow_fwd_lit);
    	arrow_side = (ImageView) context.findViewById(R.id.arrow_side_lit);
    	arrow_back = (ImageView) context.findViewById(R.id.arrow_back_lit);
    	
    	streamseq=new ImageView[] {null,		//loudness
				counter_e,
				counter_d,
				counter_c,
				counter_b,
				counter_a,
				counter_dp,
				signal_8,
				signal_7,
				signal_6,
				signal_5,
				signal_4,
				signal_3,
				signal_2,
				signal_1,
				counter_g,
				counter_f,
				band_k,
				band_ka,
				band_laser,
				arrow_fwd,
				null,		//fwdarrow again
				null,		//fwdarrow again
				arrow_side,
				null,		//sidearrow again
				null,		//sidearrow again
				arrow_back,
				null,		//backarrow again
				null,		//backarrow again
				null,		//unused
				band_x};
    	
    	isDetectorOffline = true;
	}
	
	public synchronized boolean readNewSequence(StringBuffer seq) {
		if(seq.toString().equals("$NODETECT$")) {
			isDetectorOffline=true;
			return true;
		}
		displayStr = new StringTokenizer(seq.toString(), ",");
		isDetectorOffline = false;		
		int v1tokens = displayStr.countTokens();
		if(v1tokens == 29) { 
 
			return true;
		}
		else return false;
	}
	
	public synchronized void	updateDisplay() {
		if(isDetectorOffline) {
			clearDisplay();
			return;
		}
		
		int i=0;
		while(displayStr.hasMoreTokens()) {
			String thisToken = displayStr.nextToken();
			if(streamseq[i]==null) { i++; continue; }
			if( thisToken.equals("1") ) 
				{streamseq[i].setVisibility(ImageView.VISIBLE);}
			else if( thisToken.equals("0") ) {
				streamseq[i].setVisibility(ImageView.INVISIBLE);
			}
			
			i++;
			if(i>=streamseq.length) break;
		}
	}
	
	private void clearDisplay() {
		for(int i=0; i<streamseq.length; i++)
			if(streamseq[i] != null)
				streamseq[i].setVisibility(ImageView.INVISIBLE);
	}
}
