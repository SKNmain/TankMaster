class CameraSymulation
{
    vector<Point>   tanksPosition; //pozycja czołgów
    vector<float>   tankRotation; //obrót czołgów
    vector<bool>    turnOn; //lista tego czy czołg jest włącozny czy nie
    Mat             readedImage; //obraz podłogi
    Mat             offTank; //grafika wyłączonego czołgu
    Mat             onTank; //grafika włączonego czołgu
    Mat             complitedMap; //podłoga z czołgami

    Point           mapSize; //rozmiar mapy (wielkość obrazka readedImage)

    Mat             rotateMatrix; //macierz obrotu
    Mat             rotateTank; //grafika obruconego czołgu
    Size            tankSize; //szerokość i wysokość czołgu w pikselach
    Point           rotatePoint; //punkt obrotu czołgu

    int             tankAmount; //ilość czołgów
    int             actualTank; //czołg którym aktualnie się struje

    //mruganie czołgami
    int             blinkID; //identyfikator czołgu który ma mrugać -1 oznacza że żaden
    int             detectedTanks; //ilość wykrytych czołgów
    clock_t         startBlinkTimer; //licznik czasu pomiędzy mrugnięciami
    clock_t         stopBlinkTimer;
    float           blinkTimer; //czas trwania mrugnięcia w milisekundach

    VideoCapture    capture;

    public:
        bool        camView; //czy pokazujemy obrz z kamery czy zpreparowany z zwykłych obrazków

        CameraSymulation();
        void SwitchCam(); //przełączanie się między widokiem z kamery a predefiniowanym testowym obrazkiem

        void DrawTank(); //rysowanie czołgów (tylko przy testowym obrazkiem)
        void MoveTank(int posY, int posX, int rot, int change); //poruszanie się czołgami (tylko przy testowym obrazku)

        void BlinkNext(); //włączanie następnego czołgu do mrgania
        void BlinkTank(int tankID); //wywoływanie mrugania konkretnego czołgu
        void StopBlink(); //wyłaczenie mrgania
        inline int GetBlinkID(); //pobranie który czołg aktualnie mruga

        void RefreshCamera(); //odświerzanie widoku (preparowanie obrazka lub branie klatki z kamerki)
        void GetView(Mat *mapView); //przekazanie stworzonego obrazka lub klatki z kamery dalej

};

CameraSymulation::CameraSymulation()
{
    camView = false;

    blinkID = -1;
    detectedTanks = 0;
    actualTank = 0;
    startBlinkTimer = clock();
    stopBlinkTimer = clock();
    blinkTimer = 400;

    tankAmount = 0;
    Mat frame1;

    if(camView) //zamiast preparować obrazek to czytamy sobie z kamerki
    {
        capture.open(0);
        capture.read(complitedMap);

        tankAmount = 3; //w tym miejscu powienien dostać info o podłączonych czołgach do systemu

    }
    else
    {

        //debugowe ustawianie czołgów
        Point temp(142, 122);
        tanksPosition.push_back(temp);
        temp.x = 274; temp.y = 758;
        tanksPosition.push_back(temp);
        temp.x = 1560; temp.y = 694;
        tanksPosition.push_back(temp);
        for(int i = 0; i < tanksPosition.size(); ++i)
        {
            tankRotation.push_back(0);
            turnOn.push_back(true);
        }

        Mat tempRotateMatrix(2, 3, CV_32FC1);
        rotateMatrix = tempRotateMatrix;

        //wczytanie grafik z pliku (wyjściowo będzie to kamera)
        readedImage = imread("map2.png", IMREAD_COLOR);
        offTank = imread("tankoff.png", IMREAD_COLOR);
        onTank = imread("tank.png", IMREAD_COLOR);

        tankSize = onTank.size();
        rotatePoint = Point(tankSize.width / 2.0, tankSize.height / 2.0);

        complitedMap = readedImage.clone();
        DrawTank();
    }
}

void CameraSymulation::SwitchCam()
{
    if(camView) //zamiast preparować obrazek to czytamy sobie z kamerki
    {
        capture.open(0);
        capture.read(complitedMap);

        tankAmount = 3; //w tym miejscu powienien dostać info o podłączonych czołgach do systemu

    }
    else
    {
        //debugowe ustawianie czołgów
        Point temp(142, 122);
        tanksPosition.push_back(temp);
        temp.x = 274; temp.y = 758;
        tanksPosition.push_back(temp);
        temp.x = 1560; temp.y = 694;
        tanksPosition.push_back(temp);
        for(int i = 0; i < tanksPosition.size(); ++i)
        {
            tankRotation.push_back(0);
            turnOn.push_back(true);
        }

        Mat tempRotateMatrix(2, 3, CV_32FC1);
        rotateMatrix = tempRotateMatrix;

        //wczytanie grafik z pliku (wyjściowo będzie to kamera)
        readedImage = imread("map2.png", IMREAD_COLOR);
        offTank = imread("tankoff.png", IMREAD_COLOR);
        onTank = imread("tank.png", IMREAD_COLOR);

        tankSize = onTank.size();
        rotatePoint = Point(tankSize.width / 2.0, tankSize.height / 2.0);

        complitedMap = readedImage.clone();
        DrawTank();
    }
}

void CameraSymulation::DrawTank()
{
    complitedMap = readedImage.clone();

    for(int t = 0; t < tanksPosition.size(); ++t)
    {
        ///rotacja czołgu
        rotateMatrix = getRotationMatrix2D(rotatePoint, tankRotation[t], 1); //przygotowanie macierzy rotacji
        if(turnOn[t])
        {
            warpAffine(onTank, rotateTank, rotateMatrix, onTank.size());
        }
        else warpAffine(offTank, rotateTank, rotateMatrix, offTank.size());

        ///rysowanie czołgu
        for(int i = 0; i < tankSize.height; ++i)
        {
            for(int j = 0; j < tankSize.width; ++j)
            {
                complitedMap.at<Vec3b>(Point(tanksPosition[t].x + j, tanksPosition[t].y + i)) = rotateTank.at<Vec3b>(Point(j, i));
            }
        }
    }
}

void CameraSymulation::BlinkNext() //włączanie następnego czołgu do mrgania
{
    turnOn[detectedTanks] = true;
    ++detectedTanks;
    if(detectedTanks >= tanksPosition.size())
    {
        StopBlink();
        return;
    }
    BlinkTank(detectedTanks);
}

void CameraSymulation::BlinkTank(int tankID) //wywoływanie mrugania konkretnego czołgu
{
    blinkID = tankID;
    startBlinkTimer = clock();
}
void CameraSymulation::StopBlink() //wyłaczenie mrgania
{
    turnOn[blinkID] = true;
    blinkID = -1;
}

int CameraSymulation::GetBlinkID()
{
    return blinkID;
}

void CameraSymulation::RefreshCamera() //odświerzanie widoku
{
    if(blinkID != -1)
    {
        ///mruganie
        stopBlinkTimer = clock();
        if(((stopBlinkTimer - startBlinkTimer) >= blinkTimer))
        {
            if(turnOn[blinkID])
            {
                turnOn[blinkID] = false;
                startBlinkTimer = clock();
            }
            else
            {
                turnOn[blinkID] = true;
                startBlinkTimer = clock();
            }
        }
    }

    if(camView)
    {
        capture.read(complitedMap);
    }
    else DrawTank();
}

void CameraSymulation::GetView(Mat *mapView)
{
    *mapView = complitedMap.clone();
}

void CameraSymulation::MoveTank(int posY, int posX, int rot, int change)
{
    //cout << actualTank << " " << posX << " " << posY << " " << rot << " " << change << "\n";
    if(posY != 0)
    {
        tanksPosition[actualTank].y += posY;
    }
    if(posX != 0)
    {
        tanksPosition[actualTank].x += posX;
    }
    if(rot != 0)
    {
        tankRotation[actualTank] += rot;
        if(tankRotation[actualTank] < 0) tankRotation[actualTank] += 360;
        else if(tankRotation[actualTank] > 360) tankRotation[actualTank] -= 360;
    }

    if(change > 0) //zmiana aktualnie kierowanego czołgu
    {
        ++actualTank;
        if(actualTank >= tanksPosition.size()) actualTank = 0;
    }
    else if(change < 0)
    {
        --actualTank;
        if(actualTank < 0) actualTank = tanksPosition.size() - 1;
    }
}
