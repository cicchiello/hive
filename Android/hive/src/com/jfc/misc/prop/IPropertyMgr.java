package com.jfc.misc.prop;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;

public interface IPropertyMgr {
	public AlertDialog getAlertDialog();
	public boolean onActivityResult(Activity activity, int requestCode, int resultCode, Intent intent);
}
