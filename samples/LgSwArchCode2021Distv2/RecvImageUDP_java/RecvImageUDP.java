import java.awt.GraphicsDevice;
import java.awt.GraphicsEnvironment;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.image.BufferedImage;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.net.*;
import java.io.FileOutputStream;

import javax.imageio.ImageIO;
import javax.swing.ImageIcon;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JWindow;

public class RecvImageUDP implements KeyListener {
	/* Flags and sizes */


	/*
	 * The absolute maximum datagram packet size is 65507, The maximum IP packet
	 * size of 65535 minus 20 bytes for the IP header and 8 bytes for the UDP
	 * header.
	 */
	private static int DATAGRAM_MAX_SIZE = 65507;

	/* Default values */

	public static int PORT = 3000;

	JFrame frame;


	/**
	 * Revceive method
	 * 
	 * @param port
	 *            Port
	 */
	private void receiveImages(int port) {
		boolean debug = true;

		/* Constuct frame */
		JLabel Image = new JLabel();

		frame = new JFrame("UDP Image Receiver");
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		frame.getContentPane().add(Image);
		frame.setSize(300, 10);
		frame.setVisible(true);
		frame.addKeyListener(this);

		try {

                        DatagramSocket clientsocket=new DatagramSocket(port);
			byte[] imageData = null;
			boolean sessionAvailable = false;

			/* Setup byte array to store data received */
			byte[] buffer = new byte[DATAGRAM_MAX_SIZE];

			/* Receiving loop */
			while (true) {
				/* Receive a UDP packet */
				DatagramPacket dp = new DatagramPacket(buffer, buffer.length);
				clientsocket.receive(dp);
				byte[] data = dp.getData();
                                BufferedImage image = ImageIO.read(new ByteArrayInputStream(data));
          

					Image.setIcon(new ImageIcon(image));
					frame.pack();
	

			}
		} catch (IOException e) {
			e.printStackTrace();
		} finally {

		}
	}

	/**
	 * @param args
	 */
	public static void main(String[] args) {

		/* Handle command line arguments */
		switch (args.length) {
		case 1:
			PORT = Integer.parseInt(args[0]);
		}

		RecvImageUDP  receiver = new RecvImageUDP ();
		receiver.receiveImages(PORT);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see java.awt.event.KeyListener#keyPressed(java.awt.event.KeyEvent)
	 */
	public void keyPressed(KeyEvent keyevent) {
		GraphicsDevice device = GraphicsEnvironment
				.getLocalGraphicsEnvironment().getDefaultScreenDevice();



	}

	public void keyReleased(KeyEvent keyevent) {
	}

	public void keyTyped(KeyEvent keyevent) {
	}

}
