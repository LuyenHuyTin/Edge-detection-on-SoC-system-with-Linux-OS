import cv2
import time

def edge_detection(video_path):
    # Open the video capture
    cap = cv2.VideoCapture(video_path)

    # Check if the video capture is successfully opened
    if not cap.isOpened():
        print("Error opening video capture.")
        return

    # Get the video's frames per second and frame size
    fps = cap.get(cv2.CAP_PROP_FPS)
    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

    # Create a VideoWriter object to save the processed video
    output_path = 'output.avi'  # Output video path
    fourcc = cv2.VideoWriter_fourcc(*'XVID')
    out = cv2.VideoWriter(output_path, fourcc, fps, (width, height))

    start_time = time.time()
    frame_count = 0
    while True:                             
        # Read a frame from the video capture                       
        ret, frame = cap.read()    
                                                  
        # If frame is read correctly, perform edge detection
        if ret:
            # Convert the frame to grayscale                 
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
                                            
            # Apply Canny edge detection                            
            edges = cv2.Canny(gray, 50, 150)                
                            
            # Calculate FPS                 
            frame_count += 1                              
            elapsed_time = time.time() - start_time
            fps = frame_count / elapsed_time 
                                            
            # Display FPS on the frame
            cv2.putText(edges, f"FPS: {round(fps, 2)}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (255,
                            
            # Display the resulting frame          
            cv2.imshow('Edge Detection', edges)           

            # Write the edges frame to the output video
            out.write(cv2.cvtColor(edges, cv2.COLOR_GRAY2BGR))                                      

        # Check if the 'q' key is pressed to quit
        if cv2.waitKey(1) & 0xFF == ord('q'):  
            break                                  
                                                       
    # Release the video capture and writer objects            
    cap.release()                     
    out.release()                                                                                   
                                             
    # Close all OpenCV windows           
    cv2.destroyAllWindows()                    
                                                  
    print("Edge detection completed. Output video saved as '{}'".format(output_path))
                                                              
# Main program
if __name__ == '__main__':                       
    video_path = '/dev/video0'  # Video device path
    edge_detection(video_path)

