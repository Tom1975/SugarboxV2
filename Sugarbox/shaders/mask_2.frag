uniform sampler2D texture;


void main()
{
   // récupère le pixel dans la texture
   vec2 coord = vec2(gl_TexCoord[0].x/3, (gl_TexCoord[0].y*500+23)/1024.0  /2);
   if ((mod (gl_FragCoord.x, 6.0) >= 3.0) && (mod (gl_FragCoord.y, 4.0) >= 2.0))
   {
      coord = vec2(gl_TexCoord[0].x/3, (gl_TexCoord[0].y*500+24)/1024.0 / 2);
   }
   vec4 pixel = texture2D(texture, coord);

   // 
   if (mod (gl_FragCoord.x, 3.0) >= 2.0)
   {
      pixel.r = 0; 
      pixel.g = 0;
         
   }
   else if (mod (gl_FragCoord.x, 3.0) >= 1.0)
   {
      pixel.r = 0; 
      pixel.b = 0;
   }
   else 
   {
      pixel.b = 0; 
      pixel.g = 0;
   }
   

   // et multiplication avec la couleur pour obtenir le pixel final
   gl_FragColor = gl_Color * pixel;
}