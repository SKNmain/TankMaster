#include <iostream>
#include <vector>
#include <sstream>
#include <string.h>
#include <fstream>
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/core/mat.hpp"
#include <time.h>

using namespace std;
using namespace cv;

int winSizeX = 1280;
int winSizeY = 720;

#include "function.hpp"
#include "Obstacles.hpp"
#include "TankDetector.hpp"
#include "GameManager.hpp"
#include "CameraSymulation.hpp"

int main()
{
    Mat resultImage;

    GameManager gm;

    CameraSymulation cs;

    namedWindow("GameView");

    int programcode;
    gm.SetTankCount(3);
    gm.ResetDetectors();
    gm.calibrateColor = false;

    setMouseCallback("GameView", ClickAndDragRectangle, &gm);

    cout << gm.obstaclesManager.H_MIN << " " << gm.obstaclesManager.H_MAX << "\n";
    cout << gm.obstaclesManager.S_MIN << " " << gm.obstaclesManager.S_MAX << "\n";
    cout << gm.obstaclesManager.V_MIN << " " << gm.obstaclesManager.V_MAX << "\n";

    while(true)
    {
        cs.RefreshCamera();
        cs.GetView(&resultImage);
        programcode = gm.Refresh(&resultImage, cs.GetBlinkID());

        if(programcode == 1)
        {
            cs.BlinkNext();
        }

        if(gm.GetTypeOfView() == 0 || gm.GetTypeOfView() == 3) //rzeczywisty obraz
        {

        }
        else if(gm.GetTypeOfView() == 1) //z pokazanymi pozycjami czołgu
        {
            resultImage = (*gm.GetTankImage()).clone();
        }
        else if(gm.GetTypeOfView() == 2) //przeszkody po triangulacji
        {
            resultImage = (*(gm.ShowObstacles())).clone();
        }
        else if(gm.GetTypeOfView() == 4) //przeszkody zaznaczone na biało
        {
            resultImage = (*(gm.ShowObstaclesColor())).clone();
        }
        else if(gm.GetTypeOfView() == 5) //rzeczywisty obraz z zaznaczonymi granicami
        {
            resultImage = (*gm.GetBorder(&resultImage)).clone();
        }

        resize(resultImage, resultImage, Size(winSizeX, winSizeY));
        resizeWindow("GameView", winSizeX, winSizeY);
        imshow("GameView", resultImage);

        switch(waitKey(20)) //poruszanie się kulką
        {
            //zmiana widoku
            case 49: //1 rzeczywisty obraz
                gm.Show();
            break;
            case 50: //2 rzeczywisty obraz + czołgi
                gm.ShowTanks();
            break;
            case 51: //3 przeszkody po triangulacji
                gm.ShowObstacles();
            break;
            case 52: //4 rzeczywisty obraz + statystyki
                gm.ShowStats();
            break;
            case 53: //5 przeszkody widoczne na biało
                gm.ShowObstaclesColor();
            break;
            case 54: //6 przeszkody
                gm.ShowBorder();
            break;

            //kontrola nad komponentami
            case 102: //f znajdowanie czołgów
                cs.BlinkTank(0);
            break;
            case 116: //t triangulacja
                if(gm.GetTypeOfView() == 0 || gm.GetTypeOfView() == 3)
                {
                    gm.CalibrateObstacles(resultImage);
                }
                else
                {
                    cout << "not available in this view mode\n";
                }
            break;
            case 103: //g zmiana koloru przeszkód na ten który się zaznaczy
                gm.ChangeObstaclesColor();
            break;
            case 104: //h zaznaczenie na biało przeszkód i zmiana ograniczeń kolorów
                gm.ShowObstaclesColor();
                gm.CreateTrackbars();
            break;
            case 99: //c zmiana widoku między kamerą a przygotowaną testową sceną
                if(cs.camView) cs.camView = false;
                else cs.camView = true;
                cs.SwitchCam();
            break;
            case 114: //r zmiana rozmiaru mapy
                gm.ChangeMapSize();
            break;
            case 117: //u zapis grafiki do pliku
                imwrite("./outimage.png", resultImage);
            break;

            if(!cs.camView)
            {
                //poruszanie czołgami
                case 119: //w
                    cs.MoveTank(-2, 0, 0, 0);
                break;
                case 115: //s
                    cs.MoveTank(2, 0, 0, 0);
                break;
                case 97: //a
                    cs.MoveTank(0, -2, 0, 0);
                break;
                case 100: //d
                    cs.MoveTank(0, 2, 0, 0);
                break;
                case 113: //q
                    cs.MoveTank(0, 0, 2, 0);
                break;
                case 101: //e
                    cs.MoveTank(0, 0, -2, 0);
                break;
                case 122: //z
                    cs.MoveTank(0, 0, 0, -1);
                break;
                case 120: //x
                    cs.MoveTank(0, 0, 0, 1);
                break;
            }

        }
    }


    return 0;
}
