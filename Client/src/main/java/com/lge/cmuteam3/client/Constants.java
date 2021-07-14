package com.lge.cmuteam3.client;

public class Constants {

	// CONNECTION STATE
	public static int CONNECTION_STATE_RESCANNING = 0;
	public static int CONNECTION_STATE_CONNECTING = 1;
	public static int CONNECTION_STATE_FAILED = 2;
	
	// NANO CONTROL CMD TYPE
	public static int CONTROL_TYPE_NORMAL = 0x1;

	// NANO CONTROL CMD VALUE
	public static int CONTROL_VALUE_UNKNOWN = 0x9;
	public static int CONTROL_VALUE_STOP = 0x0;
	public static int CONTROL_VALUE_RUN = 0x1;
	public static int CONTROL_VALUE_TEST_RUN = 0x2;
	public static int CONTROL_VALUE_ACCURACY = 0x3;
	public static int CONTROL_VALUE_RESCAN = 0x4;
	public static int CONTROL_VALUE_RARUN = 0x5;
}
