package com.lge.cmuteam3.client.network;

import java.net.Socket;
import com.lge.cmuteam3.client.FileProperties;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class NetworkManager {
	private static final Logger LOG = LoggerFactory.getLogger(NetworkManager.class);

	Socket nanoSocket;
	Socket controllerSocket;
	
	private static NetworkManager instance = new NetworkManager();
	private NetworkManager() {}
	public static NetworkManager getInstance() {
		if (instance == null) {
			instance = new NetworkManager();
		}
		return instance;
	}

	public Socket getNanoSocket() {
		if (nanoSocket != null) {
			LOG.info("Nano socket is busy");
			return null;
		}

		FileProperties prop = FileProperties.getInstance();
		String serverIp = prop.getProperty("server.ip");
		int serverTransferPort = Integer.parseInt(prop.getProperty("server.transfer.port"));
		int controlPort = Integer.parseInt(prop.getProperty("server.control.port"));

		try {
			LOG.info("ip:" + serverIp + " port:" + serverTransferPort);
			nanoSocket = new Socket(serverIp, serverTransferPort);
			return nanoSocket;
		} catch (Exception e) {
			LOG.info("Connection failed!!!!"+e.getMessage());
			nanoSocket = null;
			return null;
		}
	}
	
	public void controlNano() {
		
	}
}
