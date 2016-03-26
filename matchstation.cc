#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <vector>
#include <map>

struct loc {
	int a;
	int o;

	bool operator<(const loc &other) const {
		if (a < other.a) {
			return true;
		}
		if (a == other.a && o < other.o) {
			return true;
		}

		return false;
	}
};

#define FIELDS 80

struct latlon {
	float lat;
	float lon;
	double count[FIELDS];
	char *orig;

	latlon(double _lat, double _lon, double _count) {
		lat = _lat;
		lon = _lon;
		for (int i = 0; i < FIELDS; i++) {
			count[i] = _count;
		}
		orig = NULL;
	}
};

#define FOOT 0.00000274
#define BUCKET (2640 * FOOT / .8)
#define RADIUS (2640 * FOOT)

double fpow(double b, double e) {
	if (b == 0) {
		return 0;
	} else if (e == 2) {
		return b * b;
	} else {
		return exp(log(b) * e);
	}
}

double boxcox(double x, double l) {
	if (l == 0) {
		return log(x);
	}

	return (fpow(x, l) - 1) / l;
}

double pdf(double x, double a, double u, double o, double l) {
	return a * (fpow(x, l - 1)) / (boxcox(o, l) * sqrt(2 * M_PI)) * exp(- fpow(boxcox(x, l) - boxcox(u, l), 2) / (2 * fpow(boxcox(o, l), 2)));
}

void chomp(char *s) {
	for (; *s; s++) {
		if (*s == '\n') {
			*s = '\0';
		}
	}
}

int main() {
	char s[2000];

	std::multimap<loc, latlon *> map;

	FILE *f = fopen("station-corners", "r");
	while (fgets(s, 2000, f)) {
		double lat, lon;

		if (sscanf(s, "%lf %lf", &lat, &lon) != 2) {
			fprintf(stderr, "??? %s\n", s);
			continue;
		}

		struct loc loc;
		loc.a = lat / BUCKET;
		loc.o = lon / BUCKET;

		struct latlon *ll = new latlon(lat, lon, 0);
		ll->orig = strdup(s);
		chomp(ll->orig);

		map.insert(std::pair<struct loc, struct latlon *>(loc, ll));
	}
	fclose(f);

	int seq = 0;
	int opercent = -1;

	f = fopen("all-employment-2013", "r");
	while (fgets(s, 2000, f)) {
		std::vector<double> fields;

		seq++;
		int percent = seq * 100 / 6632862;
		if (percent != opercent) {
			fprintf(stderr, "%d%%\n", percent);
			opercent = percent;
		}

		char *cp = s;
		while (*cp != '\0') {
			fields.push_back(atof(cp));
			while (*cp != '\0' && *cp != ',') {
				cp++;
			}
			if (*cp == ',') {
				cp++;
			}
		}

		double lat = fields[53];
		double lon = fields[54];

		double rat = cos(lat * M_PI / 180);

		int a = lat / BUCKET;
		int o = lon / BUCKET;

		int aa, oo;
		for (aa = a - 1; aa <= a + 1; aa++) {
			for (oo = o - 1; oo <= o + 1; oo++) {
				struct loc l;
				l.a = aa;
				l.o = oo;

				std::pair<std::multimap<loc, latlon *>::iterator, std::multimap<loc, latlon *>::iterator> range;
				range = map.equal_range(l);

				for (std::multimap<loc, latlon *>::iterator it = range.first; it != range.second; ++it) {
					// printf("for %f,%f found %f,%f\n", lat, lon, it->second->lat, it->second->lon);

					double latd = lat - it->second->lat;
					double lond = (lon - it->second->lon) * rat;

					double dsq = latd * latd + lond * lond;
					if (dsq > RADIUS * RADIUS) {
						continue;
					}

					double dist = sqrt(dsq) / FOOT;

					double workweight = (pdf(dist, 133686, 990, 37.4624, 0.312) +
						             pdf(dist, 37600, 6376, 1.00334, -0.588)) /
						      (10 * (pdf(dist, 155673, 4634.18, 7.78672, 0.065896) +
							     pdf(dist, 12108.1, 38096.7, 13.4126, 0.281497)));
					double homeweight = (pdf(dist, 145726, 5859.81, 1.87102, -0.083514)) /
						      (10 * (pdf(dist, 137748, 7757, 3.8884, 0.0558907) +
							     pdf(dist, 13593.1, 39918.2, 27.3165, 0.342803)));

					for (size_t i = 0; i < FIELDS && i < fields.size(); i++) {
						double weight = workweight;
						if (i == 57) {
							weight = homeweight;
						}

						it->second->count[i] += fields[i] * weight;
					}
				}
			}
		}
	}

	for (std::multimap<loc, latlon *>::iterator it = map.begin(); it != map.end(); ++it) {
		printf("%f,%f", it->second->lat, it->second->lon);
		for (int i = 1; i < FIELDS; i++) {
			printf(" %f", it->second->count[i]);
		}
		printf(" %s\n", it->second->orig);
	}
}
