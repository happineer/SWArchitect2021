package com.lge.cmuteam3.client;

import com.lge.cmuteam3.client.ui.BaseFrame;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.awt.image.BufferedImage;
import java.util.Timer;
import java.util.TimerTask;
import javax.swing.*;

public class Player {
	private static final Logger LOG = LoggerFactory.getLogger(Player.class);

	private final BaseFrame frame;
	private JLabel video;
	private JTextArea logArea;

	// below fields will be loaded by client.properties file
	private String serverIp;
	private int serverTransferPort = 5000;
	private int delay = 80;

	private Receiver receiver;
	private TimerTask task;

	private class Scheduler extends TimerTask {

		@Override
		public void run() {
			BufferedImage image = receiver.getImageFrame();
			if (image != null) {
				LOG.debug("image taken. remain buffer : " + receiver.getRemainBufferSize());
				video.setIcon(new ImageIcon(image));
				frame.pack();
			} else {
				LOG.debug("image is not ready");
				try {
					Thread.sleep(delay);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			}
		}
	}

	public Player(BaseFrame frame) {
		this.frame = frame;

		FileProperties prop = FileProperties.getInstance();

		this.serverIp = prop.getProperty("server.ip");
		this.serverTransferPort = Integer.parseInt(prop.getProperty("server.transfer.port"));
		this.delay = Integer.parseInt(prop.getProperty("client.delay"));

		this.video = this.frame.getImageView();
		this.video.setSize(1280, 720);

		this.frame.getButtonOK().addActionListener(e -> {
			LOG.debug("Event:" + e.getActionCommand());

			start();
		});

		this.frame.getDisconnectButton().addActionListener(e -> {
			LOG.debug("Event:" + e.getActionCommand());
			if (task != null) {
				task.cancel();
				task = null;
				try {
					receiver.join();
				} catch (InterruptedException e1) {
					e1.printStackTrace();
				}
				Player.this.receiver = null;
				showLog("Disconnected!");
			} else {
				showLog("Not running!");
				return;
			}
		});

		this.logArea = frame.getLogArea();
	}

	public void start() {
		playImages();
	}

	private void playImages() {
		if (task != null) {
			showLog("Already running!");
			return;
		}

		showLog("Try to connect...");
		receiver = new Receiver(serverIp, serverTransferPort);

		task = new Scheduler();
		Timer timer = new Timer();
		timer.schedule(task, 0, delay);
	}

	private void showLog(String message) {
		logArea.append(message + "\n");
		logArea.setCaretPosition(logArea.getDocument().getLength()); // �ǾƷ��� ��ũ���Ѵ�.
	}

}