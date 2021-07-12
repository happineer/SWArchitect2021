package com.lge.cmuteam3.client.ui;

import javax.swing.JButton;

import mode.Mode;

public class ModeButton extends JButton {

	private Mode mode;
	
	public ModeButton(Mode mode) {
		super(mode.getModeName());
		this.mode = mode;
	}
	
	public Mode getMode() {
		return mode;
	}
	
}
