package com.lge.cmuteam3.client;

import java.awt.image.BufferedImage;

public class Frame {

	BufferedImage frameImage;
	long latency;
	
	public Frame(BufferedImage frameImage, long initialTime) {
		this.frameImage = frameImage;
		this.latency = System.currentTimeMillis() - initialTime;
	}
	
	public BufferedImage getFrameImage() {
		return frameImage;
	}
	
	public long getLatency() {
		return latency;
	}
	
}
