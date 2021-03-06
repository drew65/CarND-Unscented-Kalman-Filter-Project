#include "ukf.h"
#include "Eigen/Dense"
#include <iostream>
#include <math.h>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/**
 * Initializes Unscented Kalman filter
 */
UKF::UKF() {
  //DEBUG loop count
  count = 0;

  ///* initially set to false, set to true in first call of ProcessMeasurement
  is_initialized_ = false;

  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;

  // initial state vector
  x_ = VectorXd(5);

  // initial covariance matrix
  P_ = MatrixXd(5, 5);

  // Process noise standard deviation longitudinal acceleration in m/s^2
  std_a_ = 0.5;

  // Process noise standard deviation yaw acceleration in rad/s^2
  std_yawdd_ = 1.0;

  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_ = 0.3;

  /**
  TODO:

  Complete the initialization. See ukf.h for other member properties.

  Hint: one or more values initialized above might be wildly off...
  */
  ///* time when the state is true, in us
  time_us_ = 0.0;

  ///* State dimension
  n_x_ = 5;

  ///* Augmented state dimension
  n_aug_ = 7;

  ///* Sigma point spreading parameter
  lambda_ = 3 - n_x_;

  ///* Weights of sigma points
  weights_ = VectorXd(2 * n_aug_ +1);

  ///* predicted sigma points matrix
  Xsig_pred_ = MatrixXd(n_x_, 2*n_aug_+1);
}

UKF::~UKF() {}

/**
 * @param {MeasurementPackage} meas_package The latest measurement data of
 * either radar or laser.
 */
void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Make sure you switch between lidar and radar
  measurements.
  */
  if (!is_initialized_) {
    /**
    TODO:

    */
    // first measurement

    x_ << 0, 0, 0, 0, 0;


    //cout << "Kalman Filter Initialization " << endl;

		//set the state with the initial location and zero velocity


		time_us_ = meas_package.timestamp_;

    if (meas_package.sensor_type_ == MeasurementPackage::RADAR) {
      /**
      Convert radar from polar to cartesian coordinates and initialize state.
      */
      VectorXd coord = VectorXd(2);
      coord(0) = cos(meas_package.raw_measurements_(1)) * meas_package.raw_measurements_(0);
      coord(1) = sin(meas_package.raw_measurements_(1)) * meas_package.raw_measurements_(0);
      //VectorXd coord = tools.Polar2Cartesian(meas_package.raw_measurements_);
      x_ << coord[0], coord[1], 0, 0, 0;
    }
    else if (meas_package.sensor_type_ == MeasurementPackage::LASER) {
      /**
      Initialize state.
      */

      x_ << meas_package.raw_measurements_[0], meas_package.raw_measurements_[1], 0, 0, 0;
    }

    // Check for zeros in the initial values of px and py
    if (fabs(x_(0)) < 0.001 and fabs(x_(1)) < 0.001) {
     x_(0) = 0.001;
     x_(1) = 0.001;
   }

    //state covariance matrix P
	  P_ << 1, 0, 0, 0, 0,
			    0, 1, 0, 0, 0,
			    0, 0, 1000, 0, 0,
			    0, 0, 0, 1000, 0,
          0, 0, 0, 0, 1;


    // done initializing, no need to predict or update
    is_initialized_ = true;

    return;
  } // End of first measurment
  count ++;
  std::cout<<"\ncount = "<<count<<"\n";
  // For all measurements folloing the first

    //Compute the time elapsed between the current and previous measurements
	   double dt = (meas_package.timestamp_ - time_us_) / 1000000.0;	//dt - expressed in seconds
	   time_us_ = meas_package.timestamp_;

    // Call Prediction
    // If timestamps are too large, use a trick to break them up
      while (dt > 0.1) {
    	  const double dt2 = 0.1;
        Prediction(dt2);
    	  dt -= dt2;
      }
      Prediction(dt);


    //Prediction(dt);

  if (meas_package.sensor_type_ == MeasurementPackage::RADAR && use_radar_ == true) {
    // Call Radar measurement
    UpdateRadar(meas_package);
  }
  if (meas_package.sensor_type_ == MeasurementPackage::LASER && use_laser_ == true) {
    // Call Lidar measurement
    UpdateLidar(meas_package);
  }





} // UKF::End of ProcessMeasurement function

/**
 * Predicts sigma points, the state, and the state covariance matrix.
 * @param {double} delta_t the change in time (in seconds) between the last
 * measurement and this one.
 */
void UKF::Prediction(double delta_t) {
  /**
  TODO:

  Complete this function! Estimate the object's location. Modify the state
  vector, x_. Predict sigma points, the state, and the state covariance matrix.
  */

  // Generate sigma points
  //create augmented mean vector
  VectorXd x_aug = VectorXd(n_aug_);

  //create augmented state covariance
  MatrixXd P_aug = MatrixXd(n_aug_, n_aug_);

  //create sigma point matrix
  MatrixXd Xsig_aug = MatrixXd(n_aug_, 2 * n_aug_ + 1);

  x_aug.head(n_x_) = x_;
  x_aug(5) = 0;
  x_aug(6) = 0;
  //create augmented mean state
  //create augmented covariance matrix
 P_aug.setZero(n_aug_,n_aug_);
 MatrixXd Q = MatrixXd(2,2);
 Q << std_a_*std_a_, 0,
      0, std_yawdd_*std_yawdd_;
 P_aug.topLeftCorner(n_x_, n_x_) = P_;
 P_aug.bottomRightCorner(2,2) = Q;

  //create square root matrix
 MatrixXd A = MatrixXd(n_aug_,n_aug_);
 A = P_aug.llt().matrixL();

  //create augmented sigma points
 Xsig_aug.col(0)  = x_aug;

  //set remaining sigma points
  for (int i = 0; i < n_aug_; i++)
  {
    Xsig_aug.col(i+1)     = x_aug + sqrt(lambda_+n_aug_) * A.col(i);
    Xsig_aug.col(i+1+n_aug_) = x_aug - sqrt(lambda_+n_aug_) * A.col(i);
  }

  // Apply transformation model to predict new sigma points
  for (int i=0; i<2*n_aug_+1; i++) {
    if (Xsig_aug(4,1) > 0.001) {
        Xsig_pred_(0,i) = Xsig_aug(0,i) + (Xsig_aug(2,i)*(sin(Xsig_aug(3,i)+Xsig_aug(4,i)*delta_t)-sin(Xsig_aug(3,i)))/Xsig_aug(4,i)) + (delta_t*delta_t/2)*cos(Xsig_aug(3,i))*Xsig_aug(5,i);
        Xsig_pred_(1,i) = Xsig_aug(1,i) + (Xsig_aug(2,i)*(-cos(Xsig_aug(3,i)+Xsig_aug(4,i)*delta_t)+cos(Xsig_aug(3,i)))/Xsig_aug(4,i)) + (delta_t*delta_t/2)*sin(Xsig_aug(3,i))*Xsig_aug(5,i);
    } else {
        Xsig_pred_(0,i) = Xsig_aug(0,i) + Xsig_aug(2,i)*cos(Xsig_aug(3,i))*delta_t + (delta_t*delta_t/2)*cos(Xsig_aug(3,i))*Xsig_aug(5,i);
        Xsig_pred_(1,i) = Xsig_aug(1,i) + Xsig_aug(2,i)*sin(Xsig_aug(3,i))*delta_t + (delta_t*delta_t/2)*sin(Xsig_aug(3,i))*Xsig_aug(5,i);
    }
        Xsig_pred_(2,i) = Xsig_aug(2,i) + delta_t * Xsig_aug(5,i);
        Xsig_pred_(3,i) = Xsig_aug(3,i) + Xsig_aug(4,i)*delta_t + (delta_t*delta_t/2)*Xsig_aug(6,i);
        Xsig_pred_(4,i) = Xsig_aug(4,i) + delta_t*Xsig_aug(6,i);
  }

// Predict mead and covariance
//set weights
  weights_(0) = lambda_/(lambda_+n_aug_);
  for(int i=1; i<2*n_aug_+1; i++) {
      weights_(i) = 0.5/(lambda_+n_aug_);
  }
  //predict state mean
  x_.fill(0.0);
  for(int i=0; i<2*n_aug_+1; i++) {
      x_ = x_ + weights_(i) * Xsig_pred_.col(i);
  }
  //predict state covariance matrix
 P_.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //iterate over sigma points

    // state difference
    VectorXd x_diff = Xsig_pred_.col(i) - x_;
    //angle normalization
    while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
    while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;

    P_ = P_ + weights_(i) * x_diff * x_diff.transpose() ;

  }



} // End of UKF::Prediction function

/**
 * Updates the state and the state covariance matrix using a laser measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateLidar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use lidar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the lidar NIS.
  */
  // Predict measurements
  ///* Radar measurement dimension
  int n_z = 2;

  //create matrix for sigma points in measurement space
  MatrixXd Zsig = MatrixXd(n_z, 2 * n_aug_ + 1);
  Zsig.fill(0.0);

  //mean predicted measurement
  VectorXd z_pred = VectorXd(n_z);
  z_pred.fill(0.0);

  //measurement covariance matrix S
  MatrixXd S = MatrixXd(n_z,n_z);
  S.fill(0.0);

  //create matrix for cross correlation Tc
  MatrixXd Tc = MatrixXd(n_x_, n_z);
  Tc.fill(0.0);

  //transform sigma points into measurement space
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points

    //sigma point predictions in process space
    Zsig(0,i) = Xsig_pred_(0,i);
    Zsig(1,i) = Xsig_pred_(1,i);
  }

  //calculate mean predicted measurement
  for(int i=0; i<2*n_aug_+1; i++) {
      z_pred = z_pred + weights_(i) * Zsig.col(i);
  }

  //measurement covariance matrix S
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points
    //residual
    VectorXd z_diff = Zsig.col(i) - z_pred;
    S = S + weights_(i) * z_diff * z_diff.transpose();
  }

  //add measurement noise covariance matrix
  MatrixXd R = MatrixXd(n_z,n_z);
  R <<    std_laspx_*std_laspx_, 0,
          0, std_laspy_*std_laspy_;
  S = S + R;

  //calculate cross correlation matrix
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points

    //residual
    VectorXd z_diff = Zsig.col(i) - z_pred;

    //state difference
    VectorXd x_diff = Xsig_pred_.col(i) - x_;

    Tc = Tc + weights_(i) * x_diff * z_diff.transpose();
  }

  //Kalman gain K;
  MatrixXd K = Tc * S.inverse();

  VectorXd z_diff = meas_package.raw_measurements_ - z_pred;

  //update state mean and covariance matrix
  x_ = x_ + K * z_diff;
  P_ = P_ - K * S * K.transpose();

  //NIS Lidar Update
  NIS_laser_ = z_diff.transpose() * S.inverse() * z_diff;
  std:cout<<"NIS_laser_ "<<NIS_laser_<<"\n";

}

/**
 * Updates the state and the state covariance matrix using a radar measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateRadar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use radar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the radar NIS.
  */

  // Predict measurements
  ///* Radar measurement dimension
  int n_z = 3;

  //create matrix for sigma points in measurement space
  MatrixXd Zsig = MatrixXd(n_z, 2 * n_aug_ + 1);

  //mean predicted measurement
  VectorXd z_pred = VectorXd(n_z);

  //radar  measurement
  //VectorXd z = VectorXd(n_z);

  //measurement covariance matrix S
  MatrixXd S = MatrixXd(n_z,n_z);

  //create matrix for cross correlation Tc
  MatrixXd Tc = MatrixXd(n_x_, n_z);

  //transform sigma points into measurement space
  for(int i=0; i<2*n_aug_+1; i++) {
    //check for zeros
    if (fabs(Xsig_pred_(0,i)) < 0.001) {
     Xsig_pred_(0,i) = 0.001;
	  }
    if (fabs(Xsig_pred_(1,i)) < 0.001) {
     Xsig_pred_(1,i) = 0.001;
   }
      Zsig(0,i) = sqrt(Xsig_pred_(0,i)*Xsig_pred_(0,i)+Xsig_pred_(1,i)*Xsig_pred_(1,i));
      double angle = atan2(Xsig_pred_(1,i),Xsig_pred_(0,i));
      Zsig(1,i) = angle;
      //check division by zero
	  if(fabs(Zsig(0,i)) < 0.0001){
		  std::cout << "Error - Division by Zero" << std::endl;
		  Zsig(2,i) = 0;
	  } else {
      Zsig(2,i) = (Xsig_pred_(0,i)*cos(Xsig_pred_(3,i))*Xsig_pred_(2,i) + Xsig_pred_(1,i)*sin(Xsig_pred_(3,i))*Xsig_pred_(2,i))/Zsig(0,i);
	  }
  }
  //calculate mean predicted measurement
  z_pred.fill(0.0);
  for(int i=0; i<2*n_aug_+1; i++) {
      z_pred = z_pred + weights_(i) * Zsig.col(i);
  }
  //calculate measurement covariance matrix S
  MatrixXd R = MatrixXd(n_z,n_z);
  R <<  std_radr_*std_radr_, 0, 0,
        0, std_radphi_*std_radphi_, 0,
        0, 0, std_radrd_*std_radrd_;
  S.fill(0.0);
  for(int i=0; i<2*n_aug_+1; i++) {
    VectorXd z_diff = Zsig.col(i) - z_pred;
    //angle normalization
    while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
    while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;
    S = S + weights_(i) * z_diff * z_diff.transpose();
  }
  S = S + R;

  //calculate cross correlation matrix
  Tc.fill(0.0);
  for (int i=0; i<2*n_aug_+1; i++) {
      VectorXd x_diff = Xsig_pred_.col(i) - x_;
      VectorXd z_diff = Zsig.col(i) - z_pred;
      //angle normalization
      while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
      while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;
      while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
      while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;
      Tc = Tc + weights_(i)*x_diff*z_diff.transpose();
  }
  //calculate Kalman gain K;
  MatrixXd K = Tc * S.inverse();
  //residual
  VectorXd z_diff = meas_package.raw_measurements_ - z_pred;
  //update state mean and covariance matrix
  x_ = x_ + K*z_diff;
  P_ = P_ - K * S * K.transpose();
  if (fabs(x_[0]) < 0.01 || fabs(x_[1]) < 0.01) {
    std::cout<<"\nx_ = \n"<<x_<<"\n";
  }

  //NIS Update
  NIS_radar_ = z_diff.transpose() * S.inverse() * z_diff;
  std:cout<<"NIS_radar_ "<<NIS_radar_<<"\n";

}  // End of UKF::UpdateRadar function
