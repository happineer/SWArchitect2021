package com.lge.cmuteam3.client.network;

public interface OnServerStateListener {
	void onReady();
	void onFail(int serverState);
}
