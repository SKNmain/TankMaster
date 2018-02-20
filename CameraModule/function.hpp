//dodatkowe funkcje
string itos(int a)
{
    stringstream ss;
    ss << a;
    return ss.str();
}

float Distance2D(Point p, Point q)
{
    return sqrt((q.x - p.x) * (q.x - p.x) + (q.y - p.y) * (q.y - p.y));
}

