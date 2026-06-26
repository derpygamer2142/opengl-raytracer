#version 330 core

#define fragCoord gl_FragCoord.xy


in vec4 gl_FragCoord;
in vec2 texCoord;

layout(location = 0) out vec4 fragColor;

uniform float iTime;
uniform vec2 iResolution;
//uniform sampler2D inputTexture;
#define OTHER

#ifdef OTHER
void main() {
	//FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
    fragColor = vec4(gl_FragCoord.xy/iResolution, 0.0f, 1.0f);
    //fragColor = texture(inputTexture, texCoord);
}
#endif

#ifndef OTHER
// Dave_Hoskins on shadertoy
vec2 hash22(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy);

}





#define ONESHOT
const float FOCALLENGTH = 350.0;
const float NUMSAMPLES = 4.0;
const float NEESAMPLES = 4.0;
// to doo doo:
// add acryonosm like:
// [] mis
// [x] nee
// [] russian roulette (not an acyontm, add w/ stuff other than a ball)


// iq
float sphIntersect( in vec3 ro, in vec3 rd, in vec3 ce, float ra )
{
    vec3 oc = ro - ce;
    float b = dot( oc, rd );
    float c = dot( oc, oc ) - ra*ra;
    float h = b*b - c;
    if( h<0.0 ) -1.0; // no intersection
    h = sqrt( h );
    
    if (-b+h < 0.0) return -1.0;
    else if (-b-h < 0.0) return -b+h;
    else return -b-h;
}

struct Intersection {
    bool hit;
    float dist;
    vec3 normal;
    vec3 ro;
    vec3 rd;
    vec3 color;
    float roughness;
    float s;
    float ior;
    float ambient;
    bool light;
};

const vec3 lightpos = vec3(80.0, 65, 120.0);
const float lightrad = 35.0;

Intersection getIntersection(vec3 ro, vec3 rd) {
    Intersection hit = Intersection(false, 9999.99, vec3(0.0), ro, rd, vec3(0.0), 0.0, 0.0, 0.0, 0.0, false);
    {
        const vec3 pos = vec3(0.0, 0.0, 200.0);
        float t = sphIntersect(ro, rd, pos, 75.0);
        if (t >= 0.0 && t < hit.dist) {
            hit.hit = true;
            hit.dist = t;
            hit.normal = normalize((ro+rd*t)-pos);
            hit.color = vec3(1.0);//vec3(0.000,0.616,1.000);
            hit.roughness = 0.725;
            hit.s = 0.5;
            hit.ior = 1.5;
            hit.ambient = 1.0;
            hit.light = false;
        }
    }
    {
        float t = sphIntersect(ro, rd, lightpos, lightrad);
        if (t >= 0.0 && t < hit.dist) {
            hit.hit = true;
            hit.dist = t;
            hit.normal = normalize((ro+rd*t)-lightpos);
            hit.color = vec3(1.000,1.000,1.000);
            hit.roughness = 0.5;
            hit.s = 0.5;
            hit.ior = 1.0;
            hit.ambient = 0.05;
            hit.light = true;
        }
    }
    
    
    return hit;
}

// generates a random vector on the hemisphere situated around the normal
vec3 hemisphereSample(vec2 seed, vec3 normal, float roughness) {
    vec2 epsilon = hash22(seed);
    
    /*
    // based on https://ameye.dev/notes/sampling-the-hemisphere/
    float theta = acos(pow(epsilon.x, 2.0/roughness*roughness - 1.0));
    float phi = 2.0*3.14159*epsilon.y;
    */
    // GGX
    float theta = atan(roughness*roughness*sqrt(epsilon.x)/sqrt(1.0-epsilon.x));
    float phi = 2.0*3.14159*epsilon.y;
    
    vec3 vector = vec3(cos(phi)*cos(theta), sin(phi)*cos(theta), sin(theta));
    
    // https://backend.orbit.dtu.dk/ws/portalfiles/portal/126824972/onb_frisvad_jgt2012_v2.pdf
    vec3 b1;
    vec3 b2;
    if (normal.z < -0.9999999) {
        b1 = vec3(0.0, -1.0, 0.0);
        b2 = vec3(-1.0, 0.0, 0.0);
    }
    float a = 1.0/(1.0 + normal.z);
    float b = -normal.x*normal.y*a;
    b1 = vec3(1.0-normal.x*normal.x*a, b, -normal.x);
    b2 = vec3(b, 1.0-normal.y*normal.y*a, -normal.y);
    
    return vector.x*b1 + vector.y*b2 + vector.z*normal;
}

vec3 sphereSample(vec2 seed) {
    // from https://pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations
    vec2 epsilon = hash22(seed);
    
    return vec3(
        cos(2.0*3.14159*epsilon.y)*2.0*sqrt(epsilon.x*(1.0-epsilon.x)),
        sin(2.0*3.14159*epsilon.y)*2.0*sqrt(epsilon.x*(1.0-epsilon.x)),
        1.0-2.0*epsilon.x
    );
}

float chi_plus(float a) {
    if (a > 0.0) return 1.0;
    return 0.0;
}

float NDF(vec3 H, vec3 N, vec3 V, vec3 L, float roughness) {
    // GGX
    float width = roughness*roughness;
    float theta_m = acos(dot(H,N)); // this seems wrong
    float param2 = width*width + tan(theta_m)*tan(theta_m);
    
    return width*width*chi_plus(dot(H,N))/
        3.14159*pow(dot(H,N),4.0)*param2*param2;
    /*
    // blinn-phong
    
    return pow(dot(H,N), 2.0/roughness*roughness - 2.0)
            /(3.14159*roughness*roughness);*/
}

float GAF(vec3 H, vec3 N, vec3 V, vec3 L, float roughness) {
    // GGX
    float width = roughness*roughness;
    float tan_theta_v = tan(acos(dot(N,V))); // this also seems wrong
    
    return chi_plus(dot(V,H)/dot(V,N))*2.0/
        (1.0 + sqrt(1.0+width*width*tan_theta_v*tan_theta_v));
    /*
    // the one from the cook-torrance paper
    
    return min(1.0,
        min(2.0*dot(H,N)*dot(N,V)/dot(V,H),
        2.0*dot(H,N)*dot(N,L)/dot(V,H))
    );*/
}

float FEF(vec3 H, vec3 V, float ior) {
    // schlick approximation
    
    float f0 = (ior-1.)*(ior-1.)/((ior+1.)*(ior+1.));
    
    return f0 + (1.-f0)*pow(1.-dot(V,H), 5.0);
}

float NEE(vec3 ro, vec2 seed) {
    vec3 pos = lightrad*sphereSample(seed) + lightpos;
    vec3 rd = normalize(pos-ro);
    Intersection hit = getIntersection(ro, rd);
    if (hit.light) {
        return hit.ambient*4.0*3.14159/hit.dist*hit.dist;
    }
}

vec3 calcBSDF(Intersection hit, vec3 incoming) {
    vec3 V = -hit.rd;
    vec3 H = normalize(incoming + V);
    return (hit.ambient +
            (1.0-hit.s)*dot(hit.normal, incoming) + hit.s*
            NDF(H, hit.normal, V, incoming, hit.roughness)
            *GAF(H, hit.normal, V, incoming, hit.roughness)
            *FEF(H, V, hit.ior)
            /(4.0*dot(hit.normal,V))
        )*hit.color;
    //return hit.color*(1.0-hit.s)*dot(hit.normal, outward);
}


void main() {
    vec3 rd = normalize(vec3(fragCoord-iResolution.xy*0.5, FOCALLENGTH));
    vec3 ro = vec3(0.0);
    fragColor = vec4(0.2);
    
    Intersection hit = getIntersection(ro, rd);
    if (hit.hit) {
        if (hit.light) {
            fragColor = vec4(1.0);
        }
        else {
            fragColor = vec4(0.0);
            ro = ro+rd*hit.dist*0.99;
            float accumulation = 0.0;
            for (float i = 1.0; i < NEESAMPLES+1.0; i += 1.0) {
                float nee = NEE(ro, fragCoord + ro.xy*i*250.0 + iTime*1025.0);

                fragColor = vec4(hit.color*mix(accumulation, nee, 1.0/i), 1.0);
            }

            
            
            for (float i = 1.0; i < NUMSAMPLES+1.0; i += 1.0) {
                vec3 incoming = hemisphereSample(fragCoord + iTime*1025.0*i + ro.xy, hit.normal, hit.roughness);//normalize(lightpos-(ro+rd*hit.dist*0.99));
                
                
                Intersection next = getIntersection(ro,reflect(rd, incoming));
                if (next.hit) {
                    vec3 hitColor = vec3(0.0);
                    if (next.light) hitColor = calcBSDF(hit, incoming);
                    
                    fragColor = mix(fragColor, vec4(hitColor,1.0), 1.0/i);
                }
            }
            

        }
        
    }
    
    
    
    #ifndef ONESHOT
    float frame = max(texelFetch(iChannel1, ivec2(1.0,1.0), 0).r, 1.0);
    fragColor = mix(texelFetch(iChannel0, ivec2(fragCoord), 0), fragColor, 1.0/frame);
    #endif
}
#endif