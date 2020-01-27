uniform sampler2D texture;


void main()
{
   // récupère le pixel dans la texture
   vec2 coord = vec2(gl_TexCoord[0].x/2, gl_TexCoord[0].y/2);
   vec4 pixel = texture2D(texture, coord);

   if (mod (gl_FragCoord.x, 2.0) >= 1.0)
   {
      if (mod (gl_FragCoord.y, 2.0) >= 1.0)
      {
         pixel.r = 0; 
         pixel.g = 0;
         
      }
      else
      {
         pixel.r = 0; 
         pixel.b = 0;
      }
   }
   else if ( mod (gl_FragCoord.y, 2.0) < 1.0)
   {
      pixel.g = 0; 
      pixel.b = 0;
   }
   else
   {
      pixel = (0,0,0,0); 
   }
   

   // et multiplication avec la couleur pour obtenir le pixel final
   gl_FragColor = gl_Color * pixel;
}