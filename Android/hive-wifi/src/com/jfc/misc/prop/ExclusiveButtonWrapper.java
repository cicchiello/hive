package com.jfc.misc.prop;

import java.util.ArrayList;
import java.util.List;

import com.jfc.apps.hive.R;

import android.view.View;
import android.widget.ImageButton;

public class ExclusiveButtonWrapper {

	private ImageButton mButton;
	private String mName;
	private boolean mTrueEnableState;
	
	static private List<ExclusiveButtonWrapper> sButtons = new ArrayList<ExclusiveButtonWrapper>();
	
	public ExclusiveButtonWrapper(ImageButton b, String name) {
		mButton = b;
		mName = name;
		mTrueEnableState = true;
		sButtons.add(this);
	}

	public void setOnClickListener(View.OnClickListener ocl) {mButton.setOnClickListener(ocl);}

	public ImageButton getImageButton() {return mButton;}
	
	public void disableButton() {
		mTrueEnableState = false;
		mButton.setImageResource(R.drawable.ic_rarrow_disabled);
		mButton.setEnabled(false);
		if (mName.equals("motor0") || mName.equals("motor1") || mName.equals("motor2")) {
			for (ExclusiveButtonWrapper b : sButtons) 
				if (b.mName.equals("audio"))
					b.disable();
		} else if (mName.equals("audio")) {
			for (ExclusiveButtonWrapper b : sButtons) 
				if (b.mName.equals("motor0") || b.mName.equals("motor1") || b.mName.equals("motor2")) 
					b.disable();
		}
	}

	public void enableButton() {
		mTrueEnableState = true;
		boolean enableMe = false;
		List<ExclusiveButtonWrapper> buttonsToPostProcess = new ArrayList<ExclusiveButtonWrapper>();
		if (mName.equals("motor0") || mName.equals("motor1") || mName.equals("motor2")) {
			for (ExclusiveButtonWrapper b : sButtons) 
				if (b.mName.equals("audio")) {
					enableMe = b.mTrueEnableState;
					buttonsToPostProcess.add(b);
				}
		} else if (mName.equals("audio")) {
			enableMe = true;
			for (ExclusiveButtonWrapper b : sButtons) 
				if (b.mName.equals("motor0") || b.mName.equals("motor1") || b.mName.equals("motor2")) {
					enableMe &= b.mTrueEnableState;
					buttonsToPostProcess.add(b);
				}
		}
		if (enableMe) {
			enable();
			for (ExclusiveButtonWrapper b : buttonsToPostProcess) 
				b.check();
		}
	}

	private void check() {
		if (mTrueEnableState) {
			boolean enableMe = false;
			if (mName.equals("motor0") || mName.equals("motor1") || mName.equals("motor2")) {
				for (ExclusiveButtonWrapper b : sButtons) 
					if (b.mName.equals("audio")) 
						enableMe = b.mTrueEnableState;
			} else if (mName.equals("audio")) {
				enableMe = true;
				for (ExclusiveButtonWrapper b : sButtons) 
					if (b.mName.equals("motor0") || b.mName.equals("motor1") || b.mName.equals("motor2")) 
						enableMe &= b.mTrueEnableState;
			}
			if (enableMe) 
				enable();
		}
	}
	
	private void disable() {
		mButton.setImageResource(R.drawable.ic_rarrow_disabled);
		mButton.setEnabled(false);
	}

	private void enable() {
		mButton.setImageResource(R.drawable.ic_rarrow);
		mButton.setEnabled(true);
	}
	
}
