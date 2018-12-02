import java.net.*;
import java.io.*;
import java.util.Scanner;

public class client{
	private final static String exitStr = "QUIT";

	public static void main(String[] args){
		System.out.println("Enter server's IP addr and port in the form of <IP>:<port>");
		Scanner scanner = null;
		String[] tokens = null;
		Socket servSock = null;
		try{
			scanner = new Scanner(System.in);
			tokens = scanner.nextLine().split(":");
			String host = tokens[0];
			int port = Integer.parseInt(tokens[1]);

			InetAddress addr = InetAddress.getByName(host);
			servSock = new Socket(addr, port);
			servSock.setSoTimeout(5000);

			BufferedWriter bw = new BufferedWriter(
					new OutputStreamWriter(servSock.getOutputStream()));
			DataInputStream dis = new DataInputStream(
					new BufferedInputStream(servSock.getInputStream()));

			System.out.println("Enter file name you want to download, each file name must be seperated by <Enter>");
			System.out.println("Enter \"QUIT\" to exit");
			while (true){
				System.out.print("file: ");
				String filename = scanner.nextLine();

				if (filename.length() == 0)
					continue;

				if (exitStr.equals(filename)){
					break;
				}

				//send filename
				bw.write(filename + "\n");
				bw.flush();
				
				//read filesize
				long filesize = dis.readLong();
				if (filesize == 0){
					System.out.println("Cannot download the file");
					continue;
				}
				System.out.println("filesize: " + filesize);

				long nBytesRead = 0;
				byte[] buff = new byte[4096];
				
				FileOutputStream fos = new FileOutputStream(filename);

				try{
					while (nBytesRead < filesize){
						//the number of byte to read 
						long needToRead = 0;
						if (filesize - nBytesRead < 4096)
							needToRead = filesize - nBytesRead;
						else
							needToRead = 4096;

						int nBytes = dis.read(buff, 0, (int)needToRead);
						nBytesRead += nBytes;
						//System.err.println("received " + nBytes + " bytes => "
						//					+ nBytesRead + "/" + filesize);

						fos.write(buff, 0, nBytes);
					}
				}catch (SocketTimeoutException ex){
					System.err.println("encountered error while reading the file at the server side");
					fos.close();
					continue;
				}
				System.out.printf("downloading '%s' done\n", filename);
				fos.close();
			}
			servSock.close();
			scanner.close();
		} catch (IOException ex){
			ex.printStackTrace();
			System.exit(-1);
		//} catch (UnknownHostException ex){
		//	ex.printStackTrace();
		//	System.exit(-1);
		}
		
	}
}
