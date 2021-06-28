package com.lge.cmuteam3.client.ui;

import javax.swing.*;
import javax.swing.border.EmptyBorder;

import java.awt.*;
import java.awt.event.*;

public class BaseFrame extends JFrame {
	private static final long serialVersionUID = -2860632918949315691L;

	private JPanel contentPane;
	private JLabel imageView;
	private JButton connectButton;
	private JButton disconnectButton;
	private JTextArea logArea;
	private JLabel lblNewLabel;
	private JScrollPane scrollPane;
	private JPanel southPanel;

	public JLabel getImageView() {
		return imageView;
	}

	public JButton getButtonOK() {
		return connectButton;
	}

	public JButton getDisconnectButton() {
		return disconnectButton;
	}

	public JTextArea getLogArea() {
		return logArea;
	}

	public BaseFrame() {
		setTitle("CMU Team 3");
		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		setBounds(100, 100, 450, 300);
		contentPane = new JPanel();
		contentPane.setBorder(new EmptyBorder(5, 5, 5, 5));
		contentPane.setLayout(new BorderLayout(0, 0));
		setContentPane(contentPane);

		imageView = new JLabel("");
		contentPane.add(imageView, BorderLayout.CENTER);

		JPanel panel = new JPanel();
		contentPane.add(panel, BorderLayout.NORTH);

		lblNewLabel = new JLabel("Run Mode");
		panel.add(lblNewLabel);

		connectButton = new JButton("Connect");
		panel.add(connectButton);

		disconnectButton = new JButton("Disconnect");
		panel.add(disconnectButton);

		logArea = new JTextArea();
		logArea.setRows(6);
		logArea.setEditable(false);

		getRootPane().setDefaultButton(connectButton);

		scrollPane = new JScrollPane(logArea);
		contentPane.add(scrollPane, BorderLayout.SOUTH);

		// call onCancel() when cross is clicked
		setDefaultCloseOperation(DO_NOTHING_ON_CLOSE);
		addWindowListener(new WindowAdapter() {
			public void windowClosing(WindowEvent e) {
				onCancel();
			}
		});

		// call onCancel() on ESCAPE
		contentPane.registerKeyboardAction(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				onCancel();
			}
		}, KeyStroke.getKeyStroke(KeyEvent.VK_ESCAPE, 0), JComponent.WHEN_ANCESTOR_OF_FOCUSED_COMPONENT);
	}

	private void onCancel() {
		// add your code here if necessary
		dispose();
	}
}
