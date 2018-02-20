class GameManager
{
    public:
        Obstacles               obstaclesManager; //obiekt od zarządzania przeszkodami
        vector<TankDetector>    tankDetectors; //detektory do badania pozycji czołgu

        ofstream                mapfile; //plik w którm będzie zapisana mapa

        //granice mapy
        Point                   beginMap;
        Point                   endMap;
        const string            ChangeMapSizeWindow = "Change Map Size";

        Mat                     borderGFX;

        //materiały
        Mat                     tempRealCamView; //widok z kamery
        Mat                     realCamViewGS; //widok z kamery przerobiony na szary
        Mat                     lastImage; //obrazek z poprzedniej klatki przerobiony na szary

        Mat                     trianguleImage;
        Mat                     tankImage; //obraz z zaznaczonymi pozycjami czołgów
        Mat                     borderImage;

        int                     tankAmount; //ilość czołgów biorących udział w grze

        int                     typeOfView; //tryb widoku: 0 rzeczywisty | 1 z podpisanymi czołgami | 2 po triangulacji | 3 zwykłe ze statystykami

        //zmienne do zanzaczenia koloru za pomocą myszki
        bool                    mouseIsDragging;
        bool                    mouseMove;
        bool                    rectangleSelected;
        vector<int>             H_ROI;
        vector<int>             S_ROI;
        vector<int>             V_ROI;
        Rect                    rectangleROI;

        Point                   currentMousePoint; //obecna pozycja myszki podczas zaznaczania
        Point                   initialClickPoint; //miejsce w którym klikneło się myszką po czym dalej się nią trzymało

        bool                    calibrateColor;

        GameManager();
        void ChangeMapSize(); //wyświetla sówaczki do ograniczania mapy
        void SetTankCount(int tankCount); //ustaw ilość czołgów
        void AddTank(); //zwiększ ilość czołgów o 1
        void ClearTank(); //wyzerój ilość czołgów
        void ResetDetectors(); //wyzerowanie detektorów czołgów
        void CreateTrackbars(); //włączanie suwaków do zmiany koloru przeszkod (wywołanie funkcji z Obstacles Managera)
        int Refresh(Mat *realCamView, int blinkID); //odświerzanie obrazu dodaje znaczniki z pozycją czołgu jeśli ma się odpowiedni widok
        void CalibrateObstacles(Mat &realCamView); //wykryj na nowo przeszkody
        void StartGame(); //wystartowanie gry
        void ReziseMap(int bX, int bY, int eX, int eY); //zmiana rozmiaru mapy
        void ReziseMap(Point b, Point e); //zmiana rozmiaru mapy

        Mat *GetTankImage(); //pobranie obrazu z zaznaczonymi pozycjami czołgów
        Mat *GetBorder(Mat *realCamView); //pobranie obrazu z zaznaczoymi ogranicznikami mapy

        void ChangeObstaclesColor(); //do zaznaczania koloru myszką

        ///funkcje od pokazywania rzeczy w oknie
        void Show(); //widok z kamery bez żadnych dodatków

        Mat *ShowObstacles(); //pokazuje przeszkody jakopołączone trójkąty
        Mat *ShowObstaclesColor(); //pokazuje przeszkody jako białe plamy

        void ShowBorder(); //widok z kamery + ograniczniki mapy
        void ShowTanks(); //obraz z kamery z zaznaczonymi pozycjami czołgów
        void ShowStats(); //widok z kamry warz z statystykami (obecnie nie mamy jeszcze statystyk meczu)

        int GetTypeOfView(); //który aktualnie typ widoku jest aktywny

};

void TrackbarMapSize(int, void*) {} //funkcja która wykonuje się gdy coś zmieniasz na suwaczku od wielkości mapy

GameManager::GameManager()
{
    typeOfView = 0;

    calibrateColor = false;
    mouseIsDragging = false;
    mouseMove = false;
    rectangleSelected = false;

    beginMap.x = 0;
    beginMap.y = 0;
    endMap.x = 1920;
    endMap.y = 1080;
    borderGFX = imread("border.png", IMREAD_COLOR);

}

void GameManager::ChangeMapSize()
{
    namedWindow(ChangeMapSizeWindow, 0);

    createTrackbar( "BegWidht", ChangeMapSizeWindow, &(beginMap.x), 1920, TrackbarMapSize);
    createTrackbar( "BegHeight", ChangeMapSizeWindow, &(beginMap.y), 1080, TrackbarMapSize);
    createTrackbar( "EndWidth", ChangeMapSizeWindow, &(endMap.x), 1920, TrackbarMapSize);
    createTrackbar( "EndHeight", ChangeMapSizeWindow, &(endMap.y), 1080, TrackbarMapSize);
}

void GameManager::SetTankCount(int tankCount)
{
    tankAmount = tankCount;
}
void GameManager::AddTank()
{
    ++tankAmount;
}
void GameManager::ClearTank()
{
    tankAmount = 0;
}

void GameManager::ResetDetectors()
{
    tankDetectors.clear();
    TankDetector temp;
    for(int i = 0; i < tankAmount; ++i)
    {
        tankDetectors.push_back(temp);
    }
}

void GameManager::CreateTrackbars() //włączanie suwaków do zmiany koloru przeszkody
{
    obstaclesManager.CreateTrackbars();
}

int GameManager::Refresh(Mat *realCamView, int blinkID) //0 nic | 1 koniec mrygania czołgu
{
    if(typeOfView == 1) //rysowanie pozycji czołgu na obrazie
    {
        tankImage = (*realCamView).clone();
        for(int i = 0; i < tankDetectors.size(); ++i)
        {
            Point temppoint = tankDetectors[i].GetTankPosition();
            string temppos = itos(temppoint.x) + " " + itos(temppoint.y);
            circle(tankImage, temppoint, 8, Scalar(0, 255, 255), 1, LINE_AA);
            putText(tankImage, temppos.c_str(), tankDetectors[i].GetTankPosition(),  FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2, 8, false);
        }
    }

    for(int i = 0; i < tankDetectors.size(); ++i)
    {
        tankDetectors[i].DetectPosition(realCamView, 8, 8);
    }


    if(blinkID != -1)
    {
        Mat differenceImage;

        cvtColor((*realCamView).clone(), realCamViewGS, COLOR_BGR2GRAY);
        absdiff(realCamViewGS, lastImage, differenceImage);

        tankDetectors[blinkID].searchMovement(differenceImage);
        if(tankDetectors[blinkID].GetTick() >= tankDetectors[blinkID].GetMaxTick())
        {
            return 1;
        }
    }
    tempRealCamView = (*realCamView).clone();
    lastImage = (*realCamView).clone();
    cvtColor(lastImage, lastImage, COLOR_BGR2GRAY);

    return 0;

}

void GameManager::CalibrateObstacles(Mat &realCamView) //wykryj na nowo przeszkody
{
    mapfile.open("tankmap.txt", ios::out);

    obstaclesManager.SetImageToDetection(realCamView);
    obstaclesManager.ObstacleDetection(mapfile);

    mapfile.close();
}

void GameManager::StartGame() //odpalanie meczu
{

}


void GameManager::ReziseMap(int bX, int bY, int eX, int eY) //zmiana rozmiaru mapy
{
    beginMap.x = bX;
    beginMap.y = bY;
    endMap.x = eX;
    endMap.y = eY;
}

void GameManager::ReziseMap(Point b, Point e) //zmiana rozmiaru mapy
{
    beginMap.x = b.x;
    beginMap.y = b.y;
    endMap.x = e.x;
    endMap.y = e.y;
}


Mat *GameManager::GetTankImage()
{
    return &tankImage;
}
Mat *GameManager::GetBorder(Mat *realCamView)
{
    borderImage = (*realCamView).clone();

    for(int i = 0; i < 10; ++i)
        {
            for(int j = 0; j < 10; ++j)
            {
                borderImage.at<Vec3b>(Point(beginMap.x + j, beginMap.y + i)) = borderGFX.at<Vec3b>(Point(j, i));
                borderImage.at<Vec3b>(Point(endMap.x + j - 10, endMap.y + i - 10)) = borderGFX.at<Vec3b>(Point(j, i));
            }
        }

    return &borderImage;
}

///funkcje od pokazywania rzeczy w oknie
void GameManager::Show()
{
    typeOfView = 0;
}
Mat *GameManager::ShowObstacles() //pokazuje przeszkody jakopołączone trójkąty
{
    typeOfView = 2;
    //cout << "test1\n";
    obstaclesManager.SetImageToDetection(tempRealCamView);
    //cout << "test2\n";
    return obstaclesManager.GetTriangleView();
}
void GameManager::ShowBorder()
{
    typeOfView = 5;
}
void GameManager::ShowTanks()
{
    typeOfView = 1;
}
void GameManager::ShowStats() //przełączanie w tryb widoku statystyk (obecnie nic nie robi bo nie mamy statystyk)
{
    typeOfView = 3;
}
Mat *GameManager::ShowObstaclesColor() //pokazuje na biało przeszkody, a reszta jest czarna
{
    obstaclesManager.SetImageToDetection(tempRealCamView);
    typeOfView = 4;
    return obstaclesManager.GetColorView();
}
int GameManager::GetTypeOfView() //który aktualnie typ widoku jest aktywny
{
    return typeOfView;
}

void GameManager::ChangeObstaclesColor() //do zaznaczania koloru myszką
{
    calibrateColor = true;
    mouseIsDragging = false;
    mouseMove = false;
    rectangleSelected = false;
}

///funkcja do zaznaczania myszką koloru
void ClickAndDragRectangle(int event, int x, int y, int flags, void* param)
{
    GameManager *gm = (GameManager*)param;

    if((*gm).calibrateColor)
    {
        Mat HSV;
        resize((*gm).tempRealCamView, HSV, Size(winSizeX, winSizeY));
        cvtColor(HSV, HSV, COLOR_BGR2HSV);

        if(event == CV_EVENT_LBUTTONDOWN && (*gm).mouseIsDragging == false) //miejsce w którym się klikneło
        {
            (*gm).initialClickPoint = Point(x, y);
            (*gm).mouseIsDragging = true;
        }
        if(event == CV_EVENT_MOUSEMOVE && (*gm).mouseIsDragging == true) //jak się przesówa
        {
            (*gm).currentMousePoint = Point(x, y);
            (*gm).mouseMove = true;
        }
        if(event == CV_EVENT_LBUTTONUP && (*gm).mouseIsDragging == true) //gdy się odkliknęło
        {
            (*gm).rectangleROI = Rect((*gm).initialClickPoint, (*gm).currentMousePoint);

            if((*gm).H_ROI.size() > 0) (*gm).H_ROI.clear();
            if((*gm).S_ROI.size() > 0) (*gm).S_ROI.clear();
            if((*gm).V_ROI.size() > 0) (*gm).V_ROI.clear();

            if ((*gm).rectangleROI.width < 1 || (*gm).rectangleROI.height < 1) cout << "Please drag a rectangle, not a line" << endl;
            else
            {
                for (int i = (*gm).rectangleROI.x; i < (*gm).rectangleROI.x + (*gm).rectangleROI.width; i++)
                {
                    //iterate through both x and y direction and save HSV values at each and every point
                    for (int j = (*gm).rectangleROI.y; j < (*gm).rectangleROI.y + (*gm).rectangleROI.height; j++)
                    {
                        //save HSV value at this point
                        (*gm).H_ROI.push_back((int)HSV.at<cv::Vec3b>(j, i)[0]);
                        (*gm).S_ROI.push_back((int)HSV.at<cv::Vec3b>(j, i)[1]);
                        (*gm).V_ROI.push_back((int)HSV.at<cv::Vec3b>(j, i)[2]);
                    }
                }
            }

            if((*gm).H_ROI.size() > 0)
            {
                (*gm).obstaclesManager.H_MIN = *(min_element((*gm).H_ROI.begin(), (*gm).H_ROI.end()));
                (*gm).obstaclesManager.H_MAX = *(max_element((*gm).H_ROI.begin(), (*gm).H_ROI.end()));
            }
            if((*gm).S_ROI.size() > 0)
            {
                (*gm).obstaclesManager.S_MIN = *(min_element((*gm).S_ROI.begin(), (*gm).S_ROI.end()));
                (*gm).obstaclesManager.S_MAX = *(max_element((*gm).S_ROI.begin(), (*gm).S_ROI.end()));
            }
            if((*gm).V_ROI.size() > 0)
            {
                (*gm).obstaclesManager.V_MIN = *(min_element((*gm).V_ROI.begin(), (*gm).V_ROI.end()));
                (*gm).obstaclesManager.V_MAX = *(max_element((*gm).V_ROI.begin(), (*gm).V_ROI.end()));
            }

            (*gm).mouseIsDragging = false;
            (*gm).mouseMove = false;
            (*gm).rectangleSelected = true;
            (*gm).calibrateColor = false;

            cout << (*gm).obstaclesManager.H_MIN << " " << (*gm).obstaclesManager.H_MAX << "\n";
            cout << (*gm).obstaclesManager.S_MIN << " " << (*gm).obstaclesManager.S_MAX << "\n";
            cout << (*gm).obstaclesManager.V_MIN << " " << (*gm).obstaclesManager.V_MAX << "\n";
        }
    }
    return;
}
