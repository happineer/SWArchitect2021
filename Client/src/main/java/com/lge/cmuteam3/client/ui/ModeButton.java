package com.lge.cmuteam3.client.ui;

import javax.swing.*;

import mode.Mode;

import java.awt.*;

public class ModeButton extends JButton {

	private Mode mode;
	
	public ModeButton(Mode mode) {
		super(mode.getModeName());
		this.mode = mode;
		setFont(new Font("Lucida Sans", Font.PLAIN, 16));
		setVerticalAlignment(SwingConstants.CENTER);
	}
	
	public Mode getMode() {
		return mode;
	}
	
}
