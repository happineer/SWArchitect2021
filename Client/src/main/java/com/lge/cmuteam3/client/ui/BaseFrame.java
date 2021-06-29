package com.lge.cmuteam3.client.ui;

import javax.swing.*;
import javax.swing.border.EmptyBorder;

import mode.ModeManager;

import java.awt.*;
import java.awt.event.*;

public class BaseFrame extends JFrame {
	private static final long serialVersionUID = -2860632918949315691L;

	private static BaseFrame instance;
	
	
	private JPanel contentPane;
	private JLabel imageView;
	private JTextArea logArea;
	private JScrollPane scrollPane;
	private JPanel southPanel;

	public JLabel getImageView() {
		return imageView;
	}

//	public JButton getButtonOK() {
//		return connectButton;
//	}
//
//	public JButton getDisconnectButton() {
//		return disconnectButton;
//	}

	public JTextArea getLogArea() {
		return logArea;
	}

	public static BaseFrame getInstance() {
		if (instance == null) {
			instance = new BaseFrame();
		}
		return instance;
	}
	
	private BaseFrame() {
		setTitle("CMU Team 3");
		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		setBounds(100, 100, 450, 300);
		contentPane = new JPanel();
		contentPane.setBorder(new EmptyBorder(5, 5, 5, 5));
		contentPane.setLayout(new BorderLayout(0, 0));
		setContentPane(contentPane);

		imageView = new JLabel("");
		contentPane.add(imageView, BorderLayout.CENTER);

		logArea = new JTextArea();
		logArea.setRows(6);
		logArea.setEditable(false);

//		getRootPane().setDefaultButton(connectButton);

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

	public void setModePanel() {
		
		JPanel panel = new JPanel();
		contentPane.add(panel, BorderLayout.NORTH);
		panel.add(new JLabel("Run Mode"));

		ModeManager modeManager = ModeManager.getInstance();
		modeManager.getModeList().forEach((mode) -> {
			JButton button = new JButton(mode.getModeName());
			button.addActionListener(e -> {
				modeManager.startMode(mode);
			});
			panel.add(button);
		});

	}
	
	public void createLog(String message) {
		logArea.append(message + "\n");
		logArea.setCaretPosition(logArea.getDocument().getLength());
	}
	
	public void appendLog(String message) {
		logArea.append(message + "\n");
		logArea.setCaretPosition(logArea.getDocument().getLength());
	}
	
}
