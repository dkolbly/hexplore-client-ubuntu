#include <stdio.h>
#include <wire/terrain.pb.h>
#include <glm/glm.hpp>

#define M_2PI   (2*M_PI)

struct Orbit {
  Orbit(wire::terrain::Orbit const& spec);

  glm::vec3 get(double solar_time);

  double radius;
  double eccentricity;
  double inclination;
  double longitude_ascending;
  double arg_periapsis;

  // we have a global epoch, the "solar_time", so this
  // sets the phase relative to the solar time
  double mean_anomaly_at_epoch;

  // the mean motion, or average angular velocity, is
  // 2*PI/period.  The period is number of days to perform
  // a complete orbit.
  double orbital_period;

  double mean_motion;
};

Orbit::Orbit(wire::terrain::Orbit const& spec)
{
  radius = spec.radius();
  eccentricity = spec.eccentricity();
  inclination = spec.inclination();
  longitude_ascending = spec.longitude_ascending();
  arg_periapsis = spec.arg_periapsis();
  mean_anomaly_at_epoch = spec.mean_anomaly_at_epoch();
  orbital_period = spec.orbital_period();
  mean_motion = M_2PI / orbital_period;
  printf("%g/%g:=mean motion %g\n", M_2PI, orbital_period, mean_motion * 180/M_PI);
}

glm::vec3 Orbit::get(double solar_time)
{
  printf("mean motion %g\n", mean_motion * 180/M_PI);
  double M = fmod(mean_anomaly_at_epoch + solar_time * mean_motion, M_2PI);

  printf("M = %g\n", M * 180/M_PI);
  double e = eccentricity;
  printf("e = %g\n", e);

  // compute the true anomaly.  This is the actual angular position
  // in the orbit, between the perihelion and the current location.
  // It is computed iteratively(!)
  // c.f. http://www.abecedarical.com/zenosamples/zs_planetorbits.html
  // but that doesn't seem to work, so try this one instead:
  //   http://www.jgiesen.de/kepler/kepler.html
  // As a first approximation:
  double E = M;
  //double E = M + e * sin(M) * (1.0 + e * cos(M));

  printf("anomaly approx = %.3f\n", E * 180/M_PI );

  double F = E - e * sin(M) - M;
  // iterate twice
  for (int i=0; i<5; i++) {
    E = E - F/(1.0 - e * cos(M));
    F = E - e * sin(E) - M;
    //E = (E - e * sin(E) - M) / (1 - e * cos(E));
    printf("anomaly refine = %.3f\n", E * 180/M_PI);
  }
  return glm::vec3(0);
}

int main(int argc, char *argv[])
{
  wire::terrain::Orbit s;
  s.set_eccentricity(0.5);
  s.set_mean_anomaly_at_epoch(27.0 / 360 * M_2PI);
  Orbit o(s);
  printf("radius %g\n", o.radius);
  printf("period %g\n", o.orbital_period);
  o.get(0);
  o.get(0.075);
  o.get(1);
}
