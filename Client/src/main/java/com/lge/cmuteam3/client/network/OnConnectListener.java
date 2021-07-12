package com.lge.cmuteam3.client.network;

public interface OnConnectListener {
    void onConnected();
    void onFailed();
    void onFrameReceived();
}
