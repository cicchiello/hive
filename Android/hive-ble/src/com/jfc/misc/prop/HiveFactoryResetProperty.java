package com.jfc.misc.prop;

import java.util.List;

import com.jfc.apps.hive.R;
import com.jfc.util.misc.DialogUtils;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;

public class HiveFactoryResetProperty implements IPropertyMgr {
	private static final String TAG = HiveFactoryResetProperty.class.getName();

	private AlertDialog alert;

	public interface Resetter {
		void reset(Context ctxt);
	};
	
	private void init(final Activity activity, ImageButton button, final int resetQuestionResid, final List<Resetter> resetters) {
    	button.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				
		    	Runnable scrubAction = new Runnable() {
		    		@Override
		    		public void run() {
		    			alert = null; 
		    			for (Resetter r : resetters) 
		    				r.reset(activity);
		    			activity.onBackPressed();
		    		}
		    	};
		    	Runnable cancelAction = new Runnable() {
					@Override
					public void run() {alert=null;}
		    	};
		    	alert = DialogUtils.createAndShowAlertDialog(activity, resetQuestionResid, 
  										    	 android.R.string.yes, scrubAction, android.R.string.cancel, cancelAction);
			}
		});
	}
	
	public HiveFactoryResetProperty(Activity activity, ImageButton button, int resetQuestionResid, List<Resetter> resetters) {
		init(activity, button, resetQuestionResid, resetters);
	}
	
	public HiveFactoryResetProperty(Activity activity, ImageButton button, List<Resetter> resetters) {
		init(activity, button, R.string.factory_reset_question, resetters);
	}

	public AlertDialog getAlertDialog() {return alert;}
	public boolean onActivityResult(int requestCode, int resultCode, Intent intent) {return false;}
	
	@Override
	public void onPermissionResult(int requestCode, String[] permissions, int[] grantResults) {}
	
}
