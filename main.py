import cv2
import mediapipe as mp
import socket
import json

mp_drawing = mp.solutions.drawing_utils
mp_drawing_styles = mp.solutions.drawing_styles
mp_pose = mp.solutions.pose


def do_capture(sock, server_address_port):
    cap = cv2.VideoCapture(0)
    with mp_pose.Pose(
            min_detection_confidence=0.5,
            min_tracking_confidence=0.5) as pose:

        while cap.isOpened():
            success, image = cap.read()
            if not success:
                print("Ignoring empty camera frame.")
                # If loading a video, use 'break' instead of 'continue'.
                continue

            # Prepare image for MediaPipe
            image.flags.writeable = False
            image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
            results = pose.process(image)

            # ---- EXTRACT AND SEND POSE DATA HERE ----
            if results.pose_landmarks:
                # Build a simple serializable structure
                landmarks = []
                for lm in results.pose_landmarks.landmark:
                    landmarks.append({
                        "x": round(lm.x, 3),
                        "y": round(lm.y,3),
                        "z": round(lm.z,3)
                    })

                # Example payload: frame index could be added if needed
                payload = {
                    "landmarks": landmarks
                }

                # Serialize to JSON and send via UDP
                data_bytes = json.dumps(payload).encode("utf-8")
                sock.sendto(data_bytes, server_address_port)
            # -----------------------------------------

            # Draw the pose annotation on the image.
            image.flags.writeable = True
            image = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
            mp_drawing.draw_landmarks(
                image,
                results.pose_landmarks,
                mp_pose.POSE_CONNECTIONS,
                landmark_drawing_spec=mp_drawing_styles.get_default_pose_landmarks_style())

            # Flip the image horizontally for a selfie-view display.
            cv2.imshow('MediaPipe Pose', cv2.flip(image, 1))
            if cv2.waitKey(5) & 0xFF == 27:
                break

    cap.release()


def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 8192)
    server_address_port = ("127.0.0.1", 12004)
    do_capture(sock, server_address_port)
    
def receive():
    server_address_port = ("127.0.0.1", 12004)
    receiver = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    receiver.bind(server_address_port)
    while True:
        data, addr = receiver.recvfrom(8192)
        print(f"Received {data!r} from {addr}")


if __name__ == "__main__":
    main()    

