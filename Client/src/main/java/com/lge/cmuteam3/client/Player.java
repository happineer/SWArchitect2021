package com.lge.cmuteam3.client;

import java.awt.image.BufferedImage;
import java.util.Timer;
import java.util.TimerTask;
import javax.swing.ImageIcon;
import javax.swing.JFrame;
import javax.swing.JLabel;

public class Player {
	JFrame frame;
	JLabel video;
	int delay = 80;
	private Receiver receiver;

	
	private class Schduler extends TimerTask {

		@Override
		public void run() {
			BufferedImage image = receiver.getImageFrame();
			if (image != null){
				System.out.println("gaenoo take. remain buffer : " + receiver.getRemainBufferSize());
				video.setIcon(new ImageIcon(image));
				frame.pack();
			} else {
				System.out.println("gaenoo take not ready");
			}
		}

	}

	public Player(Receiver receiver, int delay){
		this.receiver = receiver;
		this.delay = delay;

		video = new JLabel();

		frame = new JFrame("CMU 3");
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		frame.getContentPane().add(video);
		frame.setSize(500, 500);
		frame.setVisible(true);
	}

	public void start() {
		playImages();
	}
	
	private void playImages() {
		TimerTask task = new Schduler();
		Timer timer = new Timer();
		timer.schedule(task, 0, delay);
	}

}