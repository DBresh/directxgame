struct VS_INPUT
{
    float4 pos4D : POSITION;
    float4 color : COLOR;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

cbuffer cbFourD : register(b0)
{
    matrix wvp;
    float time;
    float3 padding;
};

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    
    // Копіюємо початкову 4D позицію
    float4 p = input.pos4D;
    
    // --- 1. 4D Обертання ---
    // Обертаємо в площині XW (вивертання навиворіт)
    float cosT = cos(time);
    float sinT = sin(time);
    
    float newX = p.x * cosT - p.w * sinT;
    float newW = p.x * sinT + p.w * cosT;
    p.x = newX;
    p.w = newW;

    // Обертаємо в площині YW (для більшого хаосу)
    float cosT2 = cos(time * 0.7);
    float sinT2 = sin(time * 0.7);
    float newY = p.y * cosT2 - p.w * sinT2;
    newW = p.y * sinT2 + p.w * cosT2;
    p.y = newY;
    p.w = newW;
    
    // --- 2. Проєкція 4D -> 3D ("Тінь") ---
    // Встановлюємо дистанцію нашої 4D "лампи" від об'єкта
    float wDistance = 3.0;
    
    // Чим далі вершина по осі W, тим меншим буде фактор (перспектива 4-го виміру)
    float factor = 1.0 / (wDistance - p.w);
    
    // Створюємо спроєктовану 3D позицію
    float3 pos3D = float3(p.x * factor, p.y * factor, p.z * factor);

    // --- 3. Стандартний 3D рендер ---
    // Масштабуємо трохи, щоб краще бачити
    pos3D *= 2.5;
    
    // Множимо на звичну матрицю камери
    output.pos = mul(float4(pos3D, 1.0), wvp);
    
    output.color = input.color;
    
    return output;
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    return input.color;
}