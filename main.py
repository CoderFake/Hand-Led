import time
import hand as htm
import cv2
import threading
import paho.mqtt.publish as publish
import queue
import math
import colorsys

pTime = 0
cap = cv2.VideoCapture(0)
detector = htm.handDetector(detectionCon = 1)
pre_status = False
pre_distance = 0
preTime = 0
check = False
send_queue = queue.Queue()

def send_mqtt_data():
    while True:
        data = send_queue.get()
        if data is not None:
            publish.single("HoangDieu", str(data) , hostname="broker.hivemq.com")
            print(data)
        send_queue.task_done()
send_thread = threading.Thread(target=send_mqtt_data)
send_thread.start()

def distance(x1, y1, x2, y2):
    return math.sqrt((x2 - x1)**2 + (y2 - y1)**2)

while True:
    ret, frame = cap.read()

    cTime = time.time()
    fps = 1 / (cTime - pTime)
    pTime = cTime

    frame = detector.findHands(frame)
    lmList = detector.findPosition(frame, draw=False)
    if len(lmList) != 0:
        cv2.line(frame, (lmList[4][1], lmList[4][2]), (lmList[8][1], lmList[8][2]), (255, 255, 0), 3)
        sorce = int(distance(lmList[0][1]/10, lmList[0][2]/10, lmList[5][1]/10, lmList[5][2]/10))
        curr_distance = int(distance(lmList[4][1]/sorce, lmList[4][2]/sorce, lmList[8][1]/sorce, lmList[8][2]/sorce))
        if lmList[12][2] < lmList[9][2]:
            if check == False:
                preTime = cTime
                check = True
            cv2.circle(frame, (lmList[12][1], lmList[12][2]), 8, (255, 255, 0), cv2.FILLED)
        if cTime - preTime > 5:
            check = False
        if abs(curr_distance - pre_distance) > 3 and lmList[5][2] < lmList[0][2] and check == True:
            if curr_distance <= 3:
                curr_distance = 0
            if lmList[0][2] > lmList[5][2] and lmList[5][2] < lmList[0][2]:
                send_queue.put(curr_distance)
            pre_distance = curr_distance
            # condition = True
        # else:
        #     condition = False
        # if pre_status != condition:
        #     send_queue.put(condition)
        #     pre_status = condition
    cv2.putText(frame, f"FPS:{int(fps)}", (0, 70), cv2.FONT_HERSHEY_PLAIN, 3, (255, 0, 0), 3)
    cv2.imshow("Finger Detection", frame)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
