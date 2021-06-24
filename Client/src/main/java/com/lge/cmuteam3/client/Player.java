package com.lge.cmuteam3.client;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.awt.*;
import java.awt.image.BufferedImage;
import java.util.Timer;
import java.util.TimerTask;
import javax.swing.*;

public class Player {
	private static final Logger LOG = LoggerFactory.getLogger(Player.class);

	private final JFrame frame;
	JPanel panel;
	JLabel video;
	int delay = 80;
	private Receiver receiver;

	private class Scheduler extends TimerTask {

		@Override
		public void run() {
			BufferedImage image = receiver.getImageFrame();
			if (image != null){
				LOG.debug("image taken. remain buffer : " + receiver.getRemainBufferSize());
				video.setIcon(new ImageIcon(image));
				frame.pack();
			} else {
				LOG.debug("image is not ready");
			}
		}

	}

	public Player(Receiver receiver, JPanel panel, JFrame frame){
		this.receiver = receiver;

		String strDelay = FileProperties.get("client.delay");
		this.delay = Integer.parseInt(strDelay);

		video = new JLabel();
		this.panel = panel;
		this.panel.add(video, new GridBagConstraints());

		this.frame = frame;
	}

	public void start() {
		playImages();
	}
	
	private void playImages() {
		TimerTask task = new Scheduler();
		Timer timer = new Timer();
		timer.schedule(task, 0, delay);
	}

}