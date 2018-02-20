class Obstacles
{
    Mat     imageToContur; //kopia obrazu z kamery na któej będzie pracowała klasa
    Mat     readyView; //wynikowy obraz z samymi konturami
    Mat     colorView; //widok koloru

    public:
        //filtr danego kolory w przestrzeni HSV
        int     H_MIN;
        int     H_MAX;
        int     S_MIN;
        int     S_MAX;
        int     V_MIN;
        int     V_MAX;

        const string ChooseColorWindow = "Choose Color";

        bool viewIsReady;

        vector<vector<Point> > obstaclesVertices; //tablica która przechowóje wartości z

        Obstacles();
        int TurnPoint(Point p0, Point p1, Point p2); //wyznaczenie prawo lub lewo skrętności jakiś 3 punktów
        void SetImageToDetection(Mat realCamView); //pobranie obrazu z zewnątrz np kamery
        void CreateTrackbars(); //wyświetlenie sówaczków do zmiany kolorów przeszkód
        vector<vector<Point> > ObstacleDetection(ofstream &outFile); //wyszukuje prrzeszkody na grafice i zapisuje punkty do pliku
        Mat *GetTriangleView(); //wypluwa obrazek z narysowanymi przeszkodami po triangulacji dostępne dopiero po urzyciu funkcji ObstacleDetection(ofstream)
        Mat *GetColorView(); //wypluwa obrazek z zaznaczonymi tylko i wyłącznie przeszkodami


};

void TrackbarObstacleColor(int, void*) {} //funkcja która wykonuje się gdy coś zmieniasz na suwaczku od kolorów przeszkód

Obstacles::Obstacles()
{
    //ustawienie domyślnego czerwonego kolory przeszkód
    H_MIN = 155;
    H_MAX = 199;
    S_MIN = 26;
    S_MAX = 189;
    V_MIN = 98;
    V_MAX = 241;

    viewIsReady = false;
}

int Obstacles::TurnPoint(Point p0, Point p1, Point p2) //wyznaczenie prawo lub lewo skrętności jakiś 3 punktów
{
    Point tempP1;
    Point tempP2;

    tempP1 = p2 - p0;
    tempP2 = p1 - p0;

    int res = tempP1.x * tempP2.y - tempP2.x * tempP1.y;

    if(res > 0) return 1; //prawo skrętny
    else if(res < 0) return -1; //lewo skrętny

    return 0; //nie skrętny
}

void Obstacles::SetImageToDetection(Mat realCamView)
{
    imageToContur = realCamView.clone();
}

void Obstacles::CreateTrackbars() //zmiana sówaczka
{
    namedWindow(ChooseColorWindow, 0);

    createTrackbar( "H_MIN", ChooseColorWindow, &H_MIN, 255, TrackbarObstacleColor);
    createTrackbar( "H_MAX", ChooseColorWindow, &H_MAX, 255, TrackbarObstacleColor);
    createTrackbar( "S_MIN", ChooseColorWindow, &S_MIN, 255, TrackbarObstacleColor);
    createTrackbar( "S_MAX", ChooseColorWindow, &S_MAX, 255, TrackbarObstacleColor);
    createTrackbar( "V_MIN", ChooseColorWindow, &V_MIN, 255, TrackbarObstacleColor);
    createTrackbar( "V_MAX", ChooseColorWindow, &V_MAX, 255, TrackbarObstacleColor);
}

//przeszukuje obraz w poszukiwaniu konturów, a następnie tworzy z nich uproszczone figury
vector<vector<Point> > Obstacles::ObstacleDetection(ofstream &outFile)
{
    cvtColor(imageToContur, imageToContur, COLOR_BGR2HSV);
    inRange(imageToContur, Scalar(H_MIN,S_MIN,V_MIN),Scalar(H_MAX,S_MAX,V_MAX), imageToContur);

    Mat canny_output;
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;

    int thresh = 100;

    GaussianBlur(imageToContur, imageToContur, Size(7,7), 1.5, 1.5);

    //usuwanie zaklucen z obrazu by bylo jak najmniej skanowania
    Canny(imageToContur, canny_output, thresh, thresh * 4, 5);
    dilate(imageToContur, imageToContur, Mat(), Point(-1,-1), 5);

    //znajdowanie konturów
    findContours(canny_output, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

    ///Draw contours
    Mat drawing = Mat::zeros( canny_output.size(), CV_8UC3 );

    for( int i = 0; i < contours.size(); ++i)
    {
        drawContours(drawing, contours, i, Scalar(255, 0, 0), 1, 8, hierarchy, 0, Point());
    }

    //cout << contours[0].size() << " " << contours[1].size() << endl;

    Mat drawingApprox = Mat::zeros( canny_output.size(), CV_8UC3 );

    ///Szkieletyzacja konturów
    vector<vector<Point> > readyApprox;
    vector<Point> approx;
    for(int k = 0; k < contours.size(); ++k)
    {
        approxPolyDP(Mat(contours[k]), approx, arcLength(Mat(contours[k]), true) * 0.02, true);

        readyApprox.push_back(approx);
    }

    for( int i = 0; i < readyApprox.size(); ++i)
    {
        drawContours(drawingApprox, readyApprox, i, Scalar(0, 0, 255), 1, 8, hierarchy, 0, Point());
    }


    ///Show in a window
    //namedWindow(WinName, CV_WINDOW_AUTOSIZE );
    //imshow(WinName, drawing);

    //namedWindow("approx", CV_WINDOW_AUTOSIZE );
    //imshow("approx", drawingApprox);

    vector<vector<int> > Convex;
    for(int i = 0; i < readyApprox.size(); ++i)
    {
        vector<int> tempConv;
        tempConv.clear();
        for(int j = 0; j < readyApprox[i].size(); ++j)
        {

            tempConv.push_back(TurnPoint(readyApprox[i][j%(readyApprox[i].size())], readyApprox[i][(j + 1)%(readyApprox[i].size())], readyApprox[i][(j + 2)%(readyApprox[i].size())]));
        }
        Convex.push_back(tempConv);
    }

    //triangulacja
    vector<vector<Point> > TriangulationPoints;
    vector<Point> tempTriangle;
    int step = 0;
    int forword = 2;

    for(int i = 0; i < readyApprox.size(); ++i)
    {
        if(readyApprox[i].size() == 3)
        {
            tempTriangle.push_back(readyApprox[i][0]);
            tempTriangle.push_back(readyApprox[i][1]);
            tempTriangle.push_back(readyApprox[i][2]);

            TriangulationPoints.push_back(tempTriangle);
            tempTriangle.clear();
            step = 0;
            forword = 2;
            continue;
        }
        else if(readyApprox[i].size() == 4)
        {
            tempTriangle.push_back(readyApprox[i][0]);
            tempTriangle.push_back(readyApprox[i][1]);
            tempTriangle.push_back(readyApprox[i][2]);
            tempTriangle.push_back(readyApprox[i][0]);
            tempTriangle.push_back(readyApprox[i][2]);
            tempTriangle.push_back(readyApprox[i][3]);
            TriangulationPoints.push_back(tempTriangle);
            tempTriangle.clear();
            step = 0;
            forword = 2;
            continue;
        }
        for(;step != forword;)
        {
            if(Convex[i][forword - 1] >= 0) //gdy jest prawoskrętna
            {
                tempTriangle.push_back(readyApprox[i][step]);
                tempTriangle.push_back(readyApprox[i][forword - 1]);
                tempTriangle.push_back(readyApprox[i][forword]);

                ++forword;
                if(forword >= readyApprox[i].size())
                {
                    forword = 0;
                }
            }
            else //gdy jest lewoskrętna
            {
                readyApprox[i].erase(readyApprox[i].begin() + (forword));
                ++forword;
                if(forword >= readyApprox[i].size())
                {
                    forword = 0;
                }

            }

            if(step == forword) //gdy przejdzie się przez całą figurę
            {
                tempTriangle.push_back(readyApprox[i][step]);
                tempTriangle.push_back(readyApprox[i][readyApprox.size()]);
                tempTriangle.push_back(readyApprox[i][readyApprox.size() - 1]);

                TriangulationPoints.push_back(tempTriangle);
                tempTriangle.clear();
                step = 0;
                forword = 2;
                break;

            }
        }

    }

    Size mainmatsize;
    mainmatsize = drawing.size();
    Mat triangleImg = Mat::zeros(mainmatsize, CV_8UC3);
    for(int i = 0; i < TriangulationPoints.size(); ++i)
    {
        for(int j = 0; j < (TriangulationPoints[i].size() / 3); ++j)
        {
            line(triangleImg, TriangulationPoints[i][j * 3], TriangulationPoints[i][j*3 + 1], Scalar(0, 255, 0), 1, LINE_8);
            line(triangleImg, TriangulationPoints[i][j * 3 + 1], TriangulationPoints[i][j * 3 + 2], Scalar(0, 255, 0), 1, LINE_8);
            line(triangleImg, TriangulationPoints[i][j * 3 + 2], TriangulationPoints[i][j * 3], Scalar(0, 255, 0), 1, LINE_8);
        }
    }

    //namedWindow("triantulacja", CV_WINDOW_AUTOSIZE );
    //imshow("triantulacja", triangleImg);

    readyView = triangleImg.clone();

    //zapisywanie do pliku
    outFile << TriangulationPoints.size() << "\n"; //ilość przeszkód

    for(int i = 0; i < TriangulationPoints.size(); ++i) //zapisanie krójkątów
    {
        outFile << "\n";
        outFile << readyApprox[i].size() << " "; //ilość wierzchołków
        outFile << (TriangulationPoints[i].size() / 3) << "\n"; //ilosc trójkątów
        for(int j = 0; j < readyApprox[i].size(); ++j)
        {
            outFile << readyApprox[i][j].x << " " << readyApprox[i][j].y << "\n";
        }
        for(int j = 0; j < TriangulationPoints[i].size(); ++j)
        {
            outFile << TriangulationPoints[i][j].x << " " << TriangulationPoints[i][j].y << "\n";
        }
    }

    cout << "triangulate done\n";
    viewIsReady = true;
    return readyApprox;
}

Mat *Obstacles::GetTriangleView()
{
    if(!viewIsReady)
    {
        cout << "image is not exist\n";
        return &imageToContur;
    }
    return &readyView;
}

Mat *Obstacles::GetColorView()
{
    Mat HSV;
    cvtColor(imageToContur, HSV, COLOR_BGR2HSV);
    inRange(HSV, Scalar(H_MIN, S_MIN, V_MIN),Scalar(H_MAX, S_MAX, V_MAX), colorView);
    return &colorView;
}
