package com.lge.cmuteam3.client;

import java.awt.image.BufferedImage;

public class Frame {

	BufferedImage frameImage;
	long initialTime;
	
	public Frame(BufferedImage frameImage, long initialTime) {
		this.frameImage = frameImage;
		this.initialTime = initialTime;
	}
	
	public BufferedImage getFrameImage() {
		return frameImage;
	}
	
	public long getInitialTime() {
		return initialTime;
	}
	
}
