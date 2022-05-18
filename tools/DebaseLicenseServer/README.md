### Deploy Stage

gcloud functions deploy DebaseLicenseServer-Stage --project heytoaster-com --runtime go116 --trigger-http --allow-unauthenticated --region=us-west2

### Deploy Production

gcloud functions deploy DebaseLicenseServer --project heytoaster-com --runtime go116 --trigger-http --allow-unauthenticated --region=us-west2
