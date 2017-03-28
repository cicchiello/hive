package com.jfc.misc.prop;

import android.app.AlertDialog;
import android.content.Intent;

public interface IPropertyMgr {
	public AlertDialog getAlertDialog();
	public boolean onActivityResult(int requestCode, int resultCode, Intent intent);
	public void onPermissionResult(int requestCode, String[] permissions, int[] grantResults);
}
