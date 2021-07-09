package com.lge.cmuteam3.client.network;

import java.io.BufferedOutputStream;
import java.io.IOException;
import java.net.Socket;
import com.lge.cmuteam3.client.FileProperties;
import com.lge.cmuteam3.client.Receiver;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class NetworkManager {
	private static final Logger LOG = LoggerFactory.getLogger(NetworkManager.class);

	Socket nanoSocket;
	Socket controllerSocket;
	PlaybackMonitor playbackMonitor;
	Receiver receiver;
	OnServerStateListener onServerStateListener;
	boolean isReady = false;
	
	private static NetworkManager instance;
	private NetworkManager() {}
	public static NetworkManager getInstance() {
		if (instance == null) {
			instance = new NetworkManager();
		}
		return instance;
	}
	
	public synchronized void init() {
		if (isReady) {
			serviceReady();
			return;
		}
		FileProperties prop = FileProperties.getInstance();
		String serverIp = prop.getProperty("server.ip");
		int serverTransferPort = Integer.parseInt(prop.getProperty("server.transfer.port"));
		int controlPort = Integer.parseInt(prop.getProperty("server.control.port"));

		try {
			String serverInfo = serverIp + ":" + serverTransferPort;
			LOG.info(serverInfo);
			NetworkUiLogManager.append("Try to connect : " + serverInfo);
			nanoSocket = new Socket(serverIp, serverTransferPort);
			receiver = new Receiver(nanoSocket);
			serviceReady();
			
		} catch (Exception e) {
			String msg = "Connection failed! : [" + e.getMessage() + "]";
			LOG.info(msg);
			NetworkUiLogManager.append(msg);
			serviceUnavailable(1);
		}
	}

	public void reInitialize() {
		isReady = false;
		init();
	}

	private void serviceReady() {
		isReady = true;
		if (onServerStateListener != null) {
			onServerStateListener.onReady();
		}
	}

	// TODO :define fail state (as simply as possible)
	private void serviceUnavailable(int state) {
		isReady = false;
		if (onServerStateListener != null) {
			onServerStateListener.onFail(state);
		}
	}

	public boolean isReady() {
		return isReady;
	}

	public Receiver getReceiver() {
		return receiver;
	}

	public void controlNano(int type, int value) {
		try {
			BufferedOutputStream bs = new BufferedOutputStream(nanoSocket.getOutputStream());
			bs.write(type);
			bs.write(0x0);
			bs.write(0x0);
			bs.write(0x0);
			bs.write(value);
			bs.write(0x0);
			bs.write(0x0);
			bs.write(0x0);
			bs.flush();
		} catch (IOException e) {
			serviceUnavailable(1);
			e.printStackTrace();
		}
	}
	
	public void setOnServerStateListener(OnServerStateListener onServerStateListener) {
		this.onServerStateListener = onServerStateListener;
	}
}
