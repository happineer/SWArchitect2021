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
	ConnectionMonitor connectionMonitor;
	Receiver receiver;
	boolean isReady = false;
	
	private static NetworkManager instance;
	private NetworkManager() {}
	public static NetworkManager getInstance() {
		if (instance == null) {
			instance = new NetworkManager();
			instance.init();
		}
		return instance;
	}

	public Socket getNanoSocket() {
		if (isReady && nanoSocket != null && !nanoSocket.isClosed()) {
			return nanoSocket;
		}
		init();
		return nanoSocket;
		
	}
	
	public synchronized void init() {
		FileProperties prop = FileProperties.getInstance();
		String serverIp = prop.getProperty("server.ip");
		int serverTransferPort = Integer.parseInt(prop.getProperty("server.transfer.port"));
		int controlPort = Integer.parseInt(prop.getProperty("server.control.port"));

		try {
			LOG.info("ip:" + serverIp + " port:" + serverTransferPort);
			nanoSocket = new Socket(serverIp, serverTransferPort);
			receiver = new Receiver(nanoSocket);
			connectionMonitor = new ConnectionMonitor(serverIp, controlPort);
			isReady = true;
		} catch (Exception e) {
			LOG.info("Connection failed!!!!"+e.getMessage());
			isReady = false;
		}
	}
	
	public boolean isReady() {
		return isReady;
	}
	
	public Receiver getReceiver() {
		return receiver;
	}
	
	public ConnectionMonitor getConnectionMonitor() {
		return connectionMonitor;
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
			e.printStackTrace();
		}
	}
}
