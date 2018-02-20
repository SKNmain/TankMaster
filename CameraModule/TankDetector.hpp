class TankDetector
{
    Point           pos[3]; //pozycja trakerów czołgu
    vector<float>   positionCharacter; //odległości pomiędzy poszczególnymi punktami
    vector<Point>   tankMarkerPosition;
    Rect            obr;
    int             detectTick;
    int             maxTick;

    public:
        TankDetector();
        inline int GetTick(); //zwraca ilość mrugnięć cząłgu. potrzebne do znajdowania czołgów
        inline int GetMaxTick(); //zwraca wartość zmiennej maxTick czyli ile razy musi mrugnąć czołg żeby była pewność że stoi w danym miejscu
        void searchMovement(Mat &diffImage); //mapa różnic i obraz wyjściowy. wyszukiwanie mrugających punktów
        //detekcja obiektu na podstawie sily jego swiecenia
        //kalibruje swoja pozycje na podstawie srodka ciezkosci obszaru (prostokata) lastPoint +- areaX/Y
        void DetectPosition(Mat *frame, int areaX, int areaY);
        void SetPointCharacter(); //oblicza odległości pomiędzy wszystkimi trakerami na czołgu
        vector<float> GetPointCharacter(); //zwraca odległości pomiędzy trakerami na czołgu
        void SortPoint(); //Sortowanie tak by drugi punkt był zawsze tym z tyłu
        float GetTankRotate(); //liczy obrót czołgu
        Point GetTankPosition(); //wyliczanie pozycji czołgu
};

TankDetector::TankDetector()
{
    obr = Rect(0, 0, 0, 0);
    detectTick = 0;
    maxTick = 6;

}

inline int TankDetector::GetTick()
{
   return detectTick;
}
inline int TankDetector::GetMaxTick()
{
    return maxTick;
}

//wyszukiwanie mrugających punktów
void TankDetector::searchMovement(Mat &diffImage) //mapa różnic i obraz wyjściowy
{
    if(detectTick > 6) return; //gdy wystarczająco dużo razy się mrygneło

    Mat temp = diffImage.clone();

    vector< vector<Point> > contours; //wykrywanie konturów
    vector<Vec4i> hierarchy;

    findContours(temp, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0)); //wykrywanie konturów

    if(contours.size() > 0) //gdy są jakieś kontury
    {
        if(contours.size() == 3) //jesli jest wykryta poprawna ilość markerów
        {
            if(tankMarkerPosition.size() == 3)
            {
                bool climedPoint[3] = {false, false, false};//dopasowane punkty
                Point tempp;

                for(unsigned int i = 0; i < contours.size(); ++i) //sprawdzanie dopasowania do siebie punktów
                {
                    obr = boundingRect(contours[i]);
                    tempp = Point(obr.x + obr.width / 2, obr.y + obr.height / 2);
                    for(unsigned int j = 0; j < tankMarkerPosition.size(); ++j)
                    {
                        if((!climedPoint[j]) && (Distance2D(tempp, tankMarkerPosition[j]) < 3.7))
                        {
                            climedPoint[j] = true;
                        }
                    }
                }
                if(climedPoint[0] && climedPoint[0] && climedPoint[0]) //gdy wszystkie zostały dopasowane zapisujemy, że test się udał
                {
                    ///DEBUG%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                    //cout << tankMarkerPosition << "\n";

                    ++detectTick;
                    if(detectTick >= maxTick)
                    {
                        pos[0] = tankMarkerPosition[0];
                        pos[1] = tankMarkerPosition[1];
                        pos[2] = tankMarkerPosition[2];

                        SetPointCharacter();
                        SortPoint();
                    }
                }
                else //gdy nie niedopasowano punktów zaznacza, czyli że kalibracja się nie udała
                {
                    tankMarkerPosition.clear();
                    for(unsigned int i = 0; i < contours.size(); ++i)
                    {
                        obr = boundingRect(contours[i]);
                        tankMarkerPosition.push_back(Point(obr.x + obr.width / 2, obr.y + obr.height / 2));
                    }
                    detectTick = 0;
                }

            }
            else if(tankMarkerPosition.size() == 0) //za pierwszym wejściem w sumie powinno być własnie to
            {
                for(unsigned int i = 0; i < contours.size(); ++i)
                {
                    obr = boundingRect(contours[i]);
                    tankMarkerPosition.push_back(Point(obr.x + obr.width / 2, obr.y + obr.height / 2));
                }
                ++detectTick;
            }
        }
    }
    return;
}

//detekcja obiektu na podstawie sily jego swiecenia
//kalibruje swoja pozycje na podstawie srodka ciezkosci obszaru (prostokata) lastPoint +- areaX/Y
void TankDetector::DetectPosition(Mat *frame, int areaX, int areaY)
{
    //Mat frameGrey = (*frame).clone();
    //cvtColor(frameGrey, frameGrey, COLOR_BGR2GRAY);
    //cvtColor(frameGrey, frameGrey, COLOR_GRAY2BGR);

    Vec3b tempCol = 0; //wartość koloru zczytywanego z tablicy
    float factor = 0; //współczynnik świecania danej pozycji
    float factorSum = 0; //suma wszystkich współczynników
    float x = 0; //nowa obliczana pozycja X
    float y = 0; //nowa obliczana pozycja y

    for(int pointID = 0; pointID < 3; ++pointID)
    {
        tempCol = 0;
        factor = 0;
        factorSum = 0;
        x = 0;
        y = 0;

        for(int i = 0; i < (areaY * 2 - 1); ++i)
        {
            for(int j = 0; j < (areaX * 2 - 1); ++j)
            {
                if((pos[pointID].x + j - areaX >= 0) && (pos[pointID].y + i - areaY >= 0))
                {
                    tempCol = (*frame).at<Vec3b>(Point(pos[pointID].x + j - areaX, pos[pointID].y + i - areaY)); //zczytanie koloru
                    factor = float(tempCol.val[0] + tempCol.val[1] + tempCol.val[2]) / 255;

                    x += j * factor; //obliczanie mocy świecania koordynatów
                    y += i * factor;
                    factorSum += factor; //suma wszystkich współczynników
                }
            }
        }

        x /= factorSum;
        y /= factorSum; //średnia warzona wartości pozycji i wszystkich współczyników

        pos[pointID].x = pos[pointID].x + x - areaX; //zapisanie nowej pozycji i korekcja wyniku o badany obszar
        pos[pointID].y = pos[pointID].y + y - areaY;

        if(pos[pointID].x < 0) pos[pointID].x = 0; //pilnowanie wyjścia z tablicy
        if(pos[pointID].y < 0) pos[pointID].y = 0;
    }
    return;
}

void TankDetector::SetPointCharacter()
{
    vector<Point> pointCharacterXY;
    positionCharacter.clear();

    Point temp;
    temp.x = 0;
    temp.y = 0;
    pointCharacterXY.push_back(temp); //pierwszy punkt londuje na pozycji <0;0>

    temp.x = pos[1].x - pos[0].x;
    temp.y = pos[1].y - pos[0].y;
    pointCharacterXY.push_back(temp); //drugi punkt jest różnicą między pierwszym a drugim punktem

    temp.x = pos[2].x - pos[0].x;
    temp.y = pos[2].y - pos[0].y;
    pointCharacterXY.push_back(temp); //drugi punkt jest różnicą między pierwszym a trzecim punktem

    positionCharacter.push_back(Distance2D(pointCharacterXY[0], pointCharacterXY[1]));
    positionCharacter.push_back(Distance2D(pointCharacterXY[1], pointCharacterXY[2]));
    positionCharacter.push_back(Distance2D(pointCharacterXY[2], pointCharacterXY[0]));
    //  p1  d0   p2
    //   d2    d1
    //      p3
}

vector<float> TankDetector::GetPointCharacter()
{
    return positionCharacter;
}

void TankDetector::SortPoint() //Sortowanie tak by drugi punkt był zawsze tym z tyłu
{
    Point temp;
    if((Distance2D(pos[0], pos[1]) >  Distance2D(pos[1], pos[2])) && (Distance2D(pos[0], pos[1]) >  Distance2D(pos[2], pos[0]))) //1 jest długi
    {
        temp = pos[0];
        pos[0] = pos[2];
        pos[2] = temp;
    }
    else if((Distance2D(pos[1], pos[2]) >  Distance2D(pos[0], pos[1])) && (Distance2D(pos[1], pos[2]) >  Distance2D(pos[0], pos[2]))) //2 jest długi
    {
        temp = pos[1];
        pos[1] = pos[2];
        pos[2] = temp;
    }
}

float TankDetector::GetTankRotate() //liczy obrót czołgu
{
    float rotAngle = 0;

    rotAngle = atan2((pos[2].y - pos[0].y),(pos[2].x - pos[0].x)) * 180 / 3.14;
    if(rotAngle < 0) rotAngle = 360 + rotAngle; //za połową
    rotAngle /= 360; //sprowadzenie wartości do zakresu <0;1)

    return rotAngle;
}

Point TankDetector::GetTankPosition() //wyliczanie pozycji czołgu
{
    Point tankPos;
    tankPos.x = pos[0].x + pos[1].x + pos[2].x;
    tankPos.x /= 3;
    tankPos.y = pos[0].y + pos[1].y + pos[2].y;
    tankPos.y /= 3;

    //cout << "current position: " << tankPos.x << ";" << tankPos.y << "\n";

    return tankPos;
}
