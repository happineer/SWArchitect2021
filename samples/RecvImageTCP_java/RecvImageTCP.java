import java.awt.GraphicsDevice;
import java.awt.GraphicsEnvironment;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.image.BufferedImage;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.net.*;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.nio.ByteBuffer;

import javax.imageio.ImageIO;
import javax.swing.ImageIcon;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JWindow;

public class RecvImageTCP implements KeyListener {
	/* Flags and sizes */

	/* Default values */


	JFrame frame;


	/**
	 * Revceive method
	 * 

	 */
	private void receiveImages(String address, int port) {
		boolean debug = true;

		/* Constuct frame */
		JLabel Image = new JLabel();

		frame = new JFrame("TCP Image Receiver");
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		frame.getContentPane().add(Image);
		frame.setSize(300, 10);
		frame.setVisible(true);
		frame.addKeyListener(this);

		try {
             Socket TCPSocket = new Socket(address, port);
			 InputStream inputStream = TCPSocket.getInputStream();
			  byte[] imageData = null;
	

			/* Receiving loop */
			while (true) {
				/* Receive a TCP packet */
                byte[] sizeAr = new byte[4];
                inputStream.readNBytes(sizeAr,0,4);
                int size = ByteBuffer.wrap(sizeAr).asIntBuffer().get();
                byte[] imageAr = new byte[size];
                inputStream.readNBytes(imageAr,0,size);

		
                BufferedImage image = ImageIO.read(new ByteArrayInputStream(imageAr));
          

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
		if (args.length!=2)
		{
		 System.out.println("Usage: RecvImageTCP <IpAdress> <Port>");
         System. exit(0); 		 
		}
        RecvImageTCP  receiver = new RecvImageTCP ();
		receiver.receiveImages(args[0],Integer.parseInt(args[1]));
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
