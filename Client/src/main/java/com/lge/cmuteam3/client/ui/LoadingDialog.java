package com.lge.cmuteam3.client.ui;

import javax.swing.*;
import java.awt.*;
import java.net.URL;

public class LoadingDialog extends JDialog {
    public LoadingDialog(JFrame frame, String name) {
		super(frame, name, false);
		getContentPane().setBackground(Color.WHITE);

    	getContentPane().setLayout(new BoxLayout(getContentPane(), BoxLayout.X_AXIS));
    	
    	JLabel loadingImage = new JLabel(name + " Processing...");
    	getContentPane().add(loadingImage);

		URL url = getClass().getClassLoader().getResource("ajax-loader.gif");
		loadingImage.setIcon(new ImageIcon(url));
		loadingImage.setIconTextGap(10);
		loadingImage.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
		setSize(300, 100);
		setUndecorated(true);
		setVisible(true);
		setLocationRelativeTo(frame);
    }
}
